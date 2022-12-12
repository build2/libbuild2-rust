#include <libbuild2/rust/rule.hxx>

#include <libbuild2/target.hxx>
#include <libbuild2/algorithm.hxx>
#include <libbuild2/diagnostics.hxx>

#include <libbuild2/rust/target.hxx>

namespace build2
{
  namespace rust
  {
    bool compile_rule::
    match (action a, target& t) const
    {
      tracer trace ("rust::compile_rule::match");

      // See if we have a source file (while "seeing through" groups).
      //
      for (prerequisite_member p: group_prerequisite_members (a, t))
      {
        if (include (a, t, p) != include_type::normal) // Excluded/ad hoc.
          continue;

        if (p.is_a<rs> ())
          return true;
      }

      l4 ([&]{trace << "no Rust source file for target " << t;});
      return false;
    }

    recipe compile_rule::
    apply (action a, target& xt) const
    {
      tracer trace ("rust::compile_rule::apply");

      file& t (xt.as<file> ());

      // Derive the file name.
      //
      // This should be similar/consistent to/with cc::link_rule.
      //
      {
        bool exe (true);

        const char* e (nullptr); // Extension.
        const char* p (nullptr); // Prefix.
        const char* s (nullptr); // Suffix.

        // @@ TODO: we need to load the corresponding bin module. Or perhaps
        //    go straight to cc? This will need target hinting, etc.
        //
#if 0
        if (auto l = t[exe ? "bin.exe.prefix" : "bin.lib.prefix"])
          p = cast<string> (l).c_str ();
        if (auto l = t[exe ? "bin.exe.suffix" : "bin.lib.suffix"])
          s = cast<string> (l).c_str ();
#endif

        if (exe)
        {
          if (r_tclass == "windows")
            e = "exe";
          else
            e = "";

          t.derive_path (e, p, s);
        }
      }

      // Inject dependency on the output directory.
      //
      inject_fsdir (a, t);

      // Match prerequisites.
      //
      match_prerequisite_members (a, t);

      switch (a)
      {
      case perform_update_id: return [this] (action a, const target& t)
        {
          return perform_update (a, t);
        };
        // @@ TODO
      case perform_clean_id:  return &perform_clean/*_depdb*/; // Standard clean.
      default:                return noop_recipe;          // Configure update.
      }
    }

    target_state compile_rule::
    perform_update (action a, const target& xt) const
    {
      tracer trace ("rust::compile_rule::perform_update");

      context& ctx (xt.ctx);

      const file& t (xt.as<file> ());
      const path& tp (t.path ());

      // Update prerequisites and determine if any render us out-of-date.
      //
      // For now we assume the first source file is the crate root.
      //
      // @@ TODO: for now we treat all the prerequisites as potentially
      //    affecting the result. We will probably want to change that to be
      //    similar to cc::link_rule.
      //
      timestamp mt (t.load_mtime ());

      pair<optional<target_state>, const rs&> pr (
        execute_prerequisites<rs> (a, t, mt));

      bool update (!pr.first);
      target_state pstate (update ? target_state::changed : *pr.first);

      const rs& s (pr.second);

      // @@ TODO: depdb (including env_checksum).

      if (!update)
        return pstate;

      // Translate paths to relative (to working directory). This results in
      // easier to read diagnostics.
      //
      path relo (relative (tp));
      path rels (relative (s.path ()));

      cstrings args {r_path.recall_string ()};

      append_options (args, r_mode);

      // Enable/disable diagnostics color unless a custom option is specified.
      //
      // Note that custom --color can be specified as both --color=X and
      // --color X.
      //
      if (!find_option ("--color", args) &&
          !find_option_prefix ("--color=", args))
      {
        // Omit --color=never if stderr is not a terminal (we know there will
        // be no color in this case and the option will just add noise, for
        // example, in build logs).
        //
        if (const char* o = (show_diag_color () ? "--color=always" :
                             stderr_term        ? "--color=never"  :
                             nullptr))
          args.push_back (o);
      }

      // Using the target name as the crate name seems appropriate.
      //
      args.push_back ("--crate-name");
      args.push_back (t.name.c_str ());

      args.push_back ("--crate-type=bin");

      args.push_back ("-o");
      args.push_back (relo.string ().c_str ());

      args.push_back (rels.string ().c_str ());
      args.push_back (nullptr);

      if (verb >= 2)
        print_process (args);
      else if (verb)
        print_diag ("rust", s, t);

      if (!ctx.dry_run)
        run (ctx, r_path, args, 1 /* finish_verbosity */);

      t.mtime (system_clock::now ());
      return target_state::changed;
    }
  }
}
