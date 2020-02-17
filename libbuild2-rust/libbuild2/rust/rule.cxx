#include <libbuild2/rust/rule.hxx>

#include <libbuild2/target.hxx>
#include <libbuild2/algorithm.hxx>
#include <libbuild2/diagnostics.hxx>

#include <libbuild2/rust/target.hxx>

namespace build2
{
  namespace rust
  {
    compile_rule::
    compile_rule (data&& d)
        : data (move (d))
    {
    }

    bool compile_rule::
    match (action a, target& t, const string&) const
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

      file& t (static_cast<file&> (xt));

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

      // @@ TODO: depdb.

      if (!update)
        return pstate;

      // Translate paths to relative (to working directory). This results in
      // easier to read diagnostics.
      //
      path relo (relative (tp));
      path rels (relative (s.path ()));

      cstrings args {r_path.recall_string ()};

      append_options (args, r_mode);

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
        text << "rust " << s;

      if (!ctx.dry_run)
        run (r_path, args);

      t.mtime (system_clock::now ());
      return target_state::changed;
    }
  }
}
