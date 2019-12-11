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
    };

    class compile_rule: public rule, virtual data
    {
    public:
      compile_rule (data&&);

      virtual bool
      match (action, target&, const string&) const override;

      virtual recipe
      apply (action, target&) const override;

      target_state
      perform_update (action, const target&) const;
    };
  }
}
