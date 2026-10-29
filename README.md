# 64kwork

A Linux/x86-64 framework for 64 KiB intros, built from shared C and NASM sources.

## Build Variants

Configure with CMake and choose a target:

```sh
cmake -S . -B build-minsz -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-minsz --target gnu_ld_script
cmake --build build-minsz --target std_upx
cmake --build build-minsz --target smol
```

- `gnu_ld_script` uses GNU ld and the project's `link.ld` layout.
- `std_upx` uses the standard GNU ELF layout. It builds an unpacked `std_upx_elf` and, if UPX is installed, creates `std_upx.upx` in the build directory. UPX 5.2.0 was tested with this target.
- `smol` uses the `tools/smol` submodule, its custom runtime resolver, and a dedicated NASM entry shim.

Initialize the `smol` submodule when cloning without recursive submodule support:

```sh
git submodule update --init --recursive
```

The `smol` target requires GCC, Python 3, and NASM. The `std_upx` target can be built without UPX, but will then produce only the uncompressed standard ELF.
