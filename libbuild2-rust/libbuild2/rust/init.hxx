#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/module.hxx>

#include <libbuild2/rust/export.hxx>

namespace build2
{
  namespace rust
  {
    extern "C" LIBBUILD2_RUST_SYMEXPORT const module_functions*
    build2_rust_load ();
  }
}
