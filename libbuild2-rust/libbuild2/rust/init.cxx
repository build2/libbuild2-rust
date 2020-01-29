#include <libbuild2/rust/init.hxx>

#include <libbuild2/scope.hxx>
#include <libbuild2/variable.hxx>
#include <libbuild2/diagnostics.hxx>

#include <libbuild2/config/utility.hxx>

#include <libbuild2/rust/module.hxx>
#include <libbuild2/rust/target.hxx>

// @@ TODO:
//
// - Documentation.
//

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
      if (&rs != &bs)
        fail (loc) << "rust.guess module must be loaded in project root";

      // Adjust module config.build save priority (compiler).
      //
      config::save_module (rs, "rust", 250);

      // Process variables.
      //
      auto& vp (rs.var_pool ());

      //-
      // `config.rust` (`strings`)
      //
      // The Rust compiler and mode options. The default is `rustc`.
      //-
      bool new_cfg;
      process_path r_path;
      strings r_mode;
      {
        auto& var (vp.insert<strings> ("config.rust", true));
        auto r (config::required (rs, var, strings {"rustc"}));

        new_cfg = r.second;

        // Split the value into the compiler path and mode.
        //
        const strings& val (cast<strings> (*r.first));
        const string& s (val.empty () ? string () : val.front ());

        path p; try { p = path (s); } catch (const invalid_path&) {}

        if (p.empty ())
          fail << "invalid path '" << s << "' in " << var;

        // Only search in PATH (specifically, omitting the current
        // executable's directory on Windows).
        //
        // Note that the process_path value will be cached so init is false.
        //
        r_path = run_search (p,
                             false       /* init */,
                             dir_path () /* fallback */,
                             true        /* path_only */);

        r_mode.assign (++val.begin (), val.end ());
      }

      extra.set_module (
        new module (
          data {
            //-
            // `rust.path` (`process_path`)
            // `rust.mode` (`strings`)
            //
            // Compiler process path and mode options. These are components of
            // the `config.rust` value.
            // -
            rs.assign ("rust.path", move (r_path)),
            rs.assign ("rust.mode", move (r_mode))
          },
          new_cfg));

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

      if (&rs != &bs)
        fail (loc) << "rust.config module must be loaded in project root";

      // Load rust.guess and share its module instance as ours.
      //
      extra.module = load_module (rs, rs, "rust.guess", loc, extra.hints);
      auto& m (extra.module_as<module> ());

      // If this is a new value (e.g., we are configuring), then print the
      // report at verbosity level 2 and up (-v).
      //
      if (verb >= (m.new_config ? 2 : 3))
      {
        diag_record dr (text);

        {
          dr << "rust " << project (rs) << '@' << rs << '\n'
             << "  rustc      " << m.r_path;
        }

        if (!m.r_mode.empty ())
        {
          dr << '\n'
             << "  mode      "; // One space short.

          for (const string& o: m.r_mode)
            dr << ' ' << o;
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

      if (&rs != &bs)
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
