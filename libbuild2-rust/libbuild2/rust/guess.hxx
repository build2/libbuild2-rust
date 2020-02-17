#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

namespace build2
{
  namespace rust
  {
    // Information extracted from the Rust compiler.
    //
    // See the corresponding rust.* variables documentation for the semantics.
    //
    struct compiler_info
    {
      semantic_version version;
      string signature;
      string checksum;
      string target;
    };

    // Note that the returned instance is cached.
    //
    const compiler_info&
    guess (const process_path&, const strings& mode);
  }
}
