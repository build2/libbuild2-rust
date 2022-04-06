#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/rule.hxx>

namespace build2
{
  namespace rust
  {
    // Cached data shared between rules and the module.
    //
    struct data
    {
      const process_path& r_path; // rust.path
      const strings&      r_mode; // rust.mode

      const target_triplet& r_target;   // rust.target
      const string&         r_tsys;     // rust.target.system
      const string&         r_tclass;   // rust.target.class

      const string env_checksum; // Environment checksum (also in rust.path).
    };

    class compile_rule: public simple_rule, virtual data
    {
    public:
      compile_rule (data&& d): data (move (d)) {}

      virtual bool
      match (action, target&) const override;

      virtual recipe
      apply (action, target&) const override;

      target_state
      perform_update (action, const target&) const;
    };
  }
}
