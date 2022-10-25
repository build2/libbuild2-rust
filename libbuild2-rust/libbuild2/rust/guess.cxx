#include <libbuild2/rust/guess.hxx>

#include <map>

#include <libbuild2/diagnostics.hxx>

namespace build2
{
  namespace rust
  {
    // Extracting rustc information requires running it which can become
    // expensive if done repeatedly. So we cache the result.
    //
    static global_cache<compiler_info> cache;

    const compiler_info&
    guess (const process_path& pp, const strings& mo)
    {
      // First check the cache.
      //
      // Note that none of the information that we cache can be affected by
      // the environment.
      //
      string key;
      {
        sha256 cs;
        cs.append (pp.effect_string ());
        append_options (cs, mo);
        key = cs.string ();

        if (const compiler_info* r = cache.find (key))
          return *r;
      }

      sha256 cs;
      compiler_info ci;

      // First extract the version.
      //
      {
        // What about localization, you may ask. That's a good question.
        // Ideally rustc would provide a stable, machine-readable format
        // (maybe JSON) for querying this information. Perhaps combined with
        // --print=target-spec-json so that we can obtain everything with a
        // single invocation; see Rust issue #38338. But for now the rustc
        // source code has no traces of localization and so we also stay in
        // a blissful ignorance of the issue.
        //
        cstrings args {pp.recall_string ()};
        append_options (args, mo); // For example, +nightly.
        args.push_back ("-V");     // Version.
        args.push_back ("-v");     // Verbose.
        args.push_back (nullptr);

        process pr (run_start (3     /* verbosity */,
                               pp,
                               args,
                               0     /* stdin */,
                               -1    /* stdout */));
        try
        {
          ifdstream is (
            move (pr.in_ofd), fdstream_mode::skip, ifdstream::badbit);

          for (string l; !eof (getline (is, l)); )
          {
            // The first line contains the version and has the following form:
            //
            // rustc 1.41.0-nightly (412f43ac5 2019-11-24)
            //
            // We also use it as a signature.
            //
            if (ci.signature.empty ())
              ci.signature = l;

            cs.append (l);
          }

          is.close ();
        }
        catch (const io_error& e)
        {
          if (run_wait (args, pr))
            fail << "unable to read " << args[0] << " -V output";

          // If the child process has failed then assume the io error was
          // caused by that and let run_finish() deal with it.
        }

        run_finish (args, pr);

        if (ci.signature.empty ())
          fail << "no " << args[0] << " -V output";

        // Parse the version.
        //
        // @@ TODO: maybe recognize -nightly as a pre-release (there are also
        //    -beta[.N?], and -dev; see the rustc_version crate as a
        //    reference). Need to first get some clarity on what could appear
        //    there.
        //
        {
          const string& l (ci.signature);

          size_t b (l.find (' '));
          if (b == string::npos)
            fail << "no " << args[0] << " version in '" << l << "'";

          size_t e (l.find (' ', ++b));
          string v (l, b, e != string::npos ? e - b : e);

          try
          {
            ci.version = semantic_version (v, semantic_version::allow_build);
          }
          catch (const invalid_argument& e)
          {
            fail << "invalid " << args[0] << " version '" << v << "': " << e;
          }
        }
      }

      // Sniff out the target from the --print=target-spec-json output.
      //
      // @@ TODO: redo using libstud-json.
      //
      {
        cstrings args {pp.recall_string ()};
        append_options (args, mo);
        args.push_back ("-Z"); args.push_back ("unstable-options");
        args.push_back ("--print=target-spec-json");
        args.push_back (nullptr);

        process pr (run_start (3     /* verbosity */,
                               pp,
                               args,
                               0     /* stdin */,
                               -1    /* stdout */));
        try
        {
          ifdstream is (
            move (pr.in_ofd), fdstream_mode::skip, ifdstream::badbit);

          for (string l; !eof (getline (is, l)); )
          {
            if (l.find ("\"llvm-target\"") != string::npos)
            {
              size_t e (l.rfind ('"'));
              size_t b (l.rfind ('"', e - 1) + 1);
              ci.target.assign (l, b, e - b);
            }

            cs.append (l);
          }

          is.close ();
        }
        catch (const io_error& e)
        {
          if (run_wait (args, pr))
            fail << "unable to read " << args[0]
                 << " --print=target-spec-json output";
        }

        run_finish (args, pr);

        if (ci.target.empty ())
          fail << "no llvm-target in " << args[0]
               << " --print=target-spec-json output";
      }

      // Parse the target into triplet (for further tests) ignoring any
      // failures.
      //
      target_triplet tt;
      try {tt = target_triplet (ci.target);} catch (const invalid_argument&) {}

      // These are taken from rustc(1) but there are probably more (see rustc
      // issues 16330, 84386).
      //
      // See also the note on environment and caching above if adding any new
      // variables.
      //
      ci.environment = {
        "RUST_TEST_THREADS",
        "RUST_TEST_NOCAPTURE",
        "RUST_MIN_STACK",
        "RUST_BACKTRACE"};

      // See similar wrangling in cc (with which we are trying to be
      // compatible).
      //
      if (tt.system == "windows-msvc")
      {
        // When targeting MSVC, rustc uses link.exe (or lld-link) which both
        // recognize these.
        //
        ci.environment.push_back ("LIB");
        ci.environment.push_back ("LINK");
        ci.environment.push_back ("_LINK_");
      }
      else if (tt.system == "darwin")
      {
        ci.environment.push_back ("MACOSX_DEPLOYMENT_TARGET");
      }
      else
      {
        // Cover the GNU linker.
        //
        ci.environment.push_back ("LD_RUN_PATH");
      }

      ci.checksum = cs.string ();

      return cache.insert (move (key), move (ci));
    }
  }
}
