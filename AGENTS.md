# AGENTS.md: Project Rules for AI Agents

## Project

A framework for creating 64 KiB intros. The final packed executable must be no larger than 65,536 bytes. The project currently targets Linux x86-64 and uses GCC or Clang, NASM (ELF64), and CMake.

## Non-Negotiable Rules

1. **Do not use libc or CRT code directly.** Compile with `-ffreestanding` and link with `-nostdlib`. Dynamic dependencies such as libX11 and libGL may load libc transitively at runtime, but project code must not call libc symbols such as `printf`, `malloc`, or `strlen`. The entry point is `_start` in `src/asm/entry.asm`; it calls `int intro_main(int argv, char *argvc)` and exits via `SYS_exit` with the return value.
2. **Size is the primary metric.** Measure each MinSizeRel artifact with `stat -c%s`. The GNU linker-script baseline is currently 5,568 bytes before packing. Estimate the byte cost before adding code, then measure it afterward.
3. **Do not include system-library headers** (except `<stddef.h>`). Define only the required opaque types and manual `extern` prototypes; see `src/c/intro_main.c`.
4. **Do not modify `link.ld` without understanding the consequences.** See “Linker Pitfalls” below.

## Build and Verification

```sh
cmake -S . -B build-minsz -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-minsz --target gnu_ld_script std_upx smol
stat -c%s build-minsz/gnu_ld_script              # linker-script baseline size
stat -c%s build-minsz/std_upx.upx                # packed standard ELF size
stat -c%s build-minsz/smol/intro.smol            # smol output size
readelf -SW build-minsz/gnu_ld_script            # sections and padding
readelf -lW build-minsz/gnu_ld_script            # program headers and LOAD layout
```

Manual runtime checks require an X11 display (including XWayland, if applicable) and a valid `DISPLAY`:

```sh
./build-minsz/gnu_ld_script                      # opens a 1024x640 window with an animated background
xdotool search --name "64kwork intro"            # get the window ID
xdotool windowactivate --sync <id> key Escape    # exits with code 0
xdotool windowclose <id>                         # also exits with code 0
```

The other runtime outputs are `build-minsz/std_upx_elf` and `build-minsz/smol/intro.smol`.

Bound process lifetime in automated checks (for example, with `timeout`); do not wait indefinitely. The expected exit code on a normal close is 0.

## Layout

```text
CMakeLists.txt      freestanding flags, X11/GL linkage, linker script
link.ld             intro-style ELF layout (see linker pitfalls)
src/asm/entry.asm   _start -> intro_main -> SYS_exit (rax=60)
src/c/intro_main.c  Xlib window, GL 4.1 core context, frame loop
doc/                architecture and roadmap documentation
```

## Linker Pitfalls

Verified with GCC 16, binutils, and glibc's dynamic loader:

- The single `PT_LOAD` must have `p_offset=0` and cover the ELF and program headers; otherwise the dynamic loader can fail at startup. `link.ld` starts sections at `0x400000 + SIZEOF_HEADERS`.
- Do not reduce `max-page-size` below 4 KiB: Linux requires `p_offset` and `p_vaddr` to be congruent modulo the page size, or execution can fail with `SIGSEGV`.
- Avoid `-N` and `-n`; with the tested binutils they imply static-linking behavior that is incompatible with this dynamic-link setup.
- With `-Wl,-T<script>`, the linker may not find transitive libX11/libGL dependencies (libc, libxcb, libGLX, libGLdispatch); `-Wl,-rpath-link,/usr/lib` is currently required.
- `-Wl,-s` is enabled for Release and MinSizeRel. Without stripping, `.symtab` and `.strtab` add about 1.4 KiB.
- PIE, stack protection, unwind tables, `.comment`, and CFI are disabled to reduce size. In particular, `-fcf-protection=none` avoids `endbr64` in every function with the tested Arch GCC.

## X11 Pitfalls

- Drain events with `while (XPending(dpy)) XNextEvent(dpy, ev)`. `XPending` reads pending events from the connection when its local queue is empty; do not assume the `XCheck*` family is a non-reading queue-only API.
- Handle `WM_DELETE_WINDOW` (`ClientMessage` with `WM_PROTOCOLS`), Escape/`q` (`KeyPress`), and `UnmapNotify`/`DestroyNotify` when the window manager or compositor closes the window without the delete protocol.
- `XEvent` is a union sized to 24 longs on the target ABI. Events are received into `long ev[24]` and interpreted through matching manual structures such as `XAnyEventMin`.
- The project uses inline-assembly syscall helpers for `write` and `nanosleep`; do not add libc calls.

## Code Consistency

- Use lowercase names (`intro_main`); preserve external ABI names such as `glXMakeCurrent`.
- Define ABI, X11, and GLX constants from their specifications; do not invent values.
- Keep comments sparse and use them only when ABI or size context is needed.
