#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/module.hxx>

#include <libbuild2/rust/export.hxx>

namespace build2
{
  namespace rust
  {
    //-
    // Module `rust` does not require bootstrapping.
    //
    // Submodules:
    //
    //   `rust.guess`  -- set variables describing the compiler, target, etc.
    //   `rust.config` -- load `rust.guess` and set the rest of the variables.
    //   `rust`        -- load `rust.config` and register targets and rules.
    //
    // The `rust` module splits the configuration process into into two parts:
    // guessing the compiler information and the actual configuration. This
    // allows adjusting configuration (say the edition or enabled experimental
    // features) base on the compiler information by first loading the guess
    // module.
    //
    //-
    extern "C" LIBBUILD2_RUST_SYMEXPORT const module_functions*
    build2_rust_load ();
  }
}
