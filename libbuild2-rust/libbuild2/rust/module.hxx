#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/module.hxx>

#include <libbuild2/rust/rule.hxx>

namespace build2
{
  namespace rust
  {
    class module: public module_base,
                  public virtual data,
                  public compile_rule
    {
    public:
      bool new_config; // New config.rust value.

      module (data&& d, bool new_cfg)
          : data (move (d)), compile_rule (move (d)), new_config (new_cfg) {}
    };
  }
}
