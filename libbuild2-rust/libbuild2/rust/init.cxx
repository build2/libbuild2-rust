#include <libbuild2/rust/init.hxx>

#include <libbuild2/scope.hxx>
#include <libbuild2/variable.hxx>
#include <libbuild2/diagnostics.hxx>

#include <libbuild2/config/utility.hxx>

#include <libbuild2/rust/guess.hxx>
#include <libbuild2/rust/module.hxx>
#include <libbuild2/rust/target.hxx>

namespace build2
{
  namespace rust
  {
    //-
    // The `rust.guess` module.
    //-
    bool
    guess_init (scope& rs,
                scope& bs,
                const location& loc,
                bool,
                bool,
                module_init_extra& extra)
    {
      tracer trace ("rust::guess_init");
      l5 ([&]{trace << "for " << bs;});

      // We only support root loading (which means there can only be one).
      //
      if (rs != bs)
        fail (loc) << "rust.guess module must be loaded in project root";

      // Adjust module config.build save priority (compiler).
      //
      config::save_module (rs, "rust", 250);

      // Configuration.
      //
      using config::lookup_config;

      auto& vp (rs.var_pool ());

      //-
      //     config.rust [strings]
      //
      // The Rust compiler and mode options. The default is `rustc`.
      //
      // The compiler mode is the place to specify options such as `+nightly`,
      // `--target`, etc., that must be in effect for the entire build and
      // that should not be overridden by the buildfiles.
      //
      // -
      bool new_cfg (false);
      process_path ppath;
      strings mode;
      {
        auto& var (vp.insert<strings> ("config.rust"));

        const auto& val (
          cast<strings> (
            lookup_config (new_cfg, rs, var, strings {"rustc"})));

        // Split the value into the compiler path and mode.
        //
        const string& s (val.empty () ? string () : val.front ());

        path p; try { p = path (s); } catch (const invalid_path&) {}

        if (p.empty ())
          fail << "invalid path '" << s << "' in " << var;

        // Only search in PATH (specifically, omitting the current
        // executable's directory on Windows).
        //
        // Note that the process_path value will be cached so init is false.
        //
        ppath = run_search (p,
                            false       /* init */,
                            dir_path () /* fallback */,
                            true        /* path_only */);

        mode.assign (++val.begin (), val.end ());
      }

      // Extract compiler information (version, target, etc).
      //
      const compiler_info& ci (guess (ppath, mode));

      // Split/canonicalize the target.
      //
      target_triplet tt;
      {
        // First see if the user asked us to use config.sub.
        //
        string ct;
        if (config_sub)
          ct = run<string> (3,
                            *config_sub,
                            ci.target.c_str (),
                            [] (string& l, bool) {return move (l);});

        try
        {
          tt = target_triplet (ct.empty () ? ci.target : ct);
        }
        catch (const invalid_argument& e)
        {
          // This is where we suggest that the user specifies --config-sub to
          // help us out.
          //
          fail << "unable to parse " << ppath << " compiler target '"
               << ci.target << "': " << e <<
            info << "consider using the --config-sub option";
        }
      }

      //-
      //     rust.path [process_path]
      //     rust.mode [strings]
      //
      // Compiler process path and mode options. These are components of
      // the `config.rust` value.
      // -
      auto& r_path (rs.assign ("rust.path", move (ppath)));
      auto& r_mode (rs.assign ("rust.mode", move (mode)));

      //-
      //
      //     rust.signature [string]
      //     rust.checksum  [string]
      //
      // Compiler signature line and checksum.
      //
      // The signature is the `--version` line that was used to extract the
      // compiler version.
      //
      // The checksum is used to detect compiler changes. It is calculated
      // based on the `--version -v` output and the target information. Note
      // that it's not bulletproof, for example, it most likely won't detect
      // that the underlying linker has changed. However, it should detect
      // most common cases, such as an upgrade to a new version or a
      // configuration change.
      //
      // Note also that the above output is obtained with the compiler mode
      // options but without the compile option. In particular, this means
      // that `--target` should (naturally) be specified as part of
      // `config.rust` and not `config.rust.coptions`.
      //
      // -
      rs.assign ("rust.signature") = ci.signature;
      rs.assign ("rust.checksum") = ci.checksum;

      //-
      //     rust.version       [string]
      //     rust.version.major [uint64]
      //     rust.version.minor [uint64]
      //     rust.version.patch [uint64]
      //     rust.version.build [string]
      //
      // Compiler version. These are extracted from the compiler's `--version`
      // output.
      //
      //-
      rs.assign ("rust.version") = ci.version.string ();
      rs.assign ("rust.version.major") = ci.version.major;
      rs.assign ("rust.version.minor") = ci.version.minor;
      rs.assign ("rust.version.patch") = ci.version.patch;
      rs.assign ("rust.version.build") = ci.version.build;

      //-
      //     rust.target         [target_triplet]
      //     rust.target.cpu     [string]
      //     rust.target.vendor  [string]
      //     rust.target.system  [string]
      //     rust.target.version [string]
      //     rust.target.class   [string]
      //
      // The compiler's target architecture triplet, either the default or as
      // specified with `--target`. Individual components are also available
      // for convenience of access. Refer to the `target-triplet` module in
      // `libbutl` for details.
      //
      // -
      rs.assign ("rust.target.cpu")     = tt.cpu;
      rs.assign ("rust.target.vendor")  = tt.vendor;
      rs.assign ("rust.target.system")  = tt.system;
      rs.assign ("rust.target.version") = tt.version;
      rs.assign ("rust.target.class")   = tt.class_;

      auto& r_target (rs.assign ("rust.target", move (tt)));

      // Cache some values in the module for easier access in the rule.
      //
      extra.set_module (
        new module (
          data {
            r_path,
            r_mode,
            r_target,
            r_target.system,
            r_target.class_},
          new_cfg,
          ci));

      return true;
    }

    //-
    // The `rust.config` module.
    //-
    bool
    config_init (scope& rs,
                 scope& bs,
                 const location& loc,
                 bool,
                 bool,
                 module_init_extra& extra)
    {
      tracer trace ("rust::const_init");
      l5 ([&]{trace << "for " << bs;});

      if (rs != bs)
        fail (loc) << "rust.config module must be loaded in project root";

      // Load rust.guess and share its module instance as ours.
      //
      extra.module = load_module (rs, rs, "rust.guess", loc, extra.hints);
      auto& m (extra.module_as<module> ());

      const compiler_info& ci (m.cinfo);
      const target_triplet& tt (m.r_target);

      // If this is a new value (e.g., we are configuring), then print the
      // report at verbosity level 2 and up (-v).
      //
      if (verb >= (m.new_config ? 2 : 3))
      {
        diag_record dr (text);

        {
          dr << "rust " << project (rs) << '@' << rs << '\n'
             << "  rustc      " << m.r_path << '\n';
        }

        if (!m.r_mode.empty ())
        {
          dr << "  mode      "; // One space short.

          for (const string& o: m.r_mode)
            dr << ' ' << o;

          dr << '\n';
        }

        {
          dr << "  version    " << ci.version.string () << '\n'
             << "  major      " << ci.version.major << '\n'
             << "  minor      " << ci.version.minor << '\n'
             << "  patch      " << ci.version.patch << '\n';
        }

        if (!ci.version.build.empty ())
        {
          dr << "  build      " << ci.version.build << '\n';
        }

        {
          const string& ct (tt.string ()); // Canonical target.

          dr << "  signature  " << ci.signature << '\n'
             << "  checksum   " << ci.checksum << '\n'
             << "  target     " << ct;

          if (ct != ci.target)
            dr << " (" << ci.target << ")";
        }
      }

      return true;
    }

    //-
    // The `rust` module.
    //-
    bool
    init (scope& rs,
          scope& bs,
          const location& loc,
          bool,
          bool,
          module_init_extra& extra)
    {
      tracer trace ("rust::init");
      l5 ([&]{trace << "for " << bs;});

      if (rs != bs)
        fail (loc) << "rust module must be loaded in project root";

      // Load rust.config and share its module instance as ours.
      //
      extra.module = load_module (rs, rs, "rust.config", loc, extra.hints);
      auto& m (extra.module_as<module> ());

      //-
      // Target types:
      //
      //   `rs{}` -- Rust source file.
      //-
      rs.insert_target_type<rust::rs> ();

      //-
      // Rules:
      //
      //   `rust.compile` -- Compile a Rust crate identified as a first `rs{}`
      //                     prerequisite.
      //-
      rs.insert_rule<exe> (perform_update_id,   "rust.compile", m);
      rs.insert_rule<exe> (perform_clean_id,    "rust.compile", m);
      rs.insert_rule<exe> (configure_update_id, "rust.compile", m);

      return true;
    }

    static const module_functions mod_functions[] =
    {
      // NOTE: don't forget to also update the documentation in init.hxx if
      //       changing anything here.

      {"rust.guess",  nullptr, guess_init},
      {"rust.config", nullptr, config_init},
      {"rust",        nullptr, init},
      {nullptr,       nullptr, nullptr}
    };

    const module_functions*
    build2_rust_load ()
    {
      return mod_functions;
    }
  }
}
