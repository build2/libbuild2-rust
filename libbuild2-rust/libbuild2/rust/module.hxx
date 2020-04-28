#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/module.hxx>

#include <libbuild2/rust/rule.hxx>
#include <libbuild2/rust/guess.hxx>

namespace build2
{
  namespace rust
  {
    class module: public build2::module,
                  public virtual data,
                  public compile_rule
    {
    public:
      bool new_cfg;               // New config.rust value.
      const compiler_info& cinfo;

      module (data&& d, bool nc, const compiler_info& ci)
          : data (move (d)),
            compile_rule (move (d)),
            new_cfg (nc),
            cinfo (ci) {}
    };
  }
}
