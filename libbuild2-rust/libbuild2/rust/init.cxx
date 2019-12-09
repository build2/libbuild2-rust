#include <libbuild2/rust/init.hxx>

#include <libbuild2/diagnostics.hxx>

using namespace std;

namespace build2
{
  namespace rust
  {
    bool
    init (scope&,
          scope&,
          const location& l,
          unique_ptr<module_base>&,
          bool,
          bool,
          const variable_map&)
    {
      info (l) << "module rust initialized";
      return true;
    }

    static const module_functions mod_functions[] =
    {
      {"rust", nullptr, init},
      {nullptr, nullptr, nullptr}
    };

    const module_functions*
    build2_rust_load ()
    {
      return mod_functions;
    }
  }
}
