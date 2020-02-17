# libbuild2-rust

Rust build system module for `build2`.

Currently requires nightly Rust (due to [Rust issue
#38338](https://github.com/rust-lang/rust/issues/38338)). You can either pass
`+nightly` as part of `config.rust`, for example:

```
$ b configure config.rust="rustc +nightly"
```

Or make the nightly build the default (it's also possible to do this per
directory, see [Managing
versions](https://doc.rust-lang.org/edition-guide/rust-2018/rustup-for-managing-rust-versions.html#managing-versions)
for details). For example:

```
$ rustup default nightly
```

## Cross-compiling for Windows

It's possible to cross-compile for the Windows target with `lld`. First
install `lld`, for example, using your system's package manager. You will also
need to copy Windows Platform SDK and Visual Studio libraries. Here we assume
the setup suggested in [`msvc-linux`](https://github.com/build2/msvc-linux)
which also allows us to run tests under Wine emulation. Alternatively, just
copy over the directories listed below.


Once this is done, use the command line along these lines adjusting the
Platform SDK and Visual Studio versions to match your setup:

```
$ b config.rust="rustc \
  --target=x86_64-pc-windows-msvc \
  -C linker=lld-link \
  -C link-arg='/LIBPATH:$HOME/.wine/drive_c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.20.27508/lib/x64' \
  -C link-arg='/LIBPATH:$HOME/.wine/drive_c/Program Files (x86)/Windows Kits/10/Lib/10.0.14393/um/x64' \
  -C link-arg='/LIBPATH:$HOME/.wine/drive_c/Program Files (x86)/Windows Kits/10/Lib/10.0.14393/ucrt/x64'"
```
