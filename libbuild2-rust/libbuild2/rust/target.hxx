#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/target.hxx>

#include <libbuild2/rust/export.hxx>

namespace build2
{
  namespace rust
  {
    class LIBBUILD2_RUST_SYMEXPORT rs: public file
    {
    public:
      rs (context& c, dir_path d, dir_path o, string n)
        : file (c, move (d), move (o), move (n))
      {
        dynamic_type = &static_type;
      }

    public:
      static const target_type static_type;
    };
  }
}
