#include <libbuild2/rust/target.hxx>

#include <libbuild2/algorithm.hxx>

namespace build2
{
  namespace rust
  {
    extern const char rs_ext_def[] = "rs";
    const target_type rs::static_type
    {
      "rs",
      &file::static_type,
      &target_factory<rs>,
      nullptr /* fixed_extension */,
      &target_extension_var<rs_ext_def>,
      &target_pattern_var<rs_ext_def>,
      nullptr /* print */,
      &file_search,
      target_type::flag::none
    };
  }
}
