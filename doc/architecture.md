# 64kwork Architecture

## Goal

A framework for Linux/x86-64 64 KiB intros. The final executable must be no larger than 65,536 bytes after packing. The project currently builds an unpacked, dynamically linked baseline without CRT startup code, with careful attention to every byte.

The project statically includes its own code but dynamically imports heavyweight libraries such as libX11 and libGL (and their implementation/driver dependencies). The libraries' code is outside the executable; import names, dynamic symbols, relocations, PLT/GOT entries, and related metadata still occupy space in the file.

## Layers

```text
intro (scene data and code, procedural generation)  <- future engine
intro_main: frame loop, events, GL context            <- src/c/intro_main.c (current base)
platform: X11 (Xlib) + GLX, syscall wrappers          <- manual prototypes, no system headers
minicrt (planned): project-owned memory/string tools  <- replaces libc calls in project code
_start (src/asm/entry.asm)                             <- sole entry point
        Linux kernel (syscalls) + ld.so (dynamic imports)
```

- `entry.asm` is the standard GNU ELF startup file. It zeroes `rdi`/`rsi` (the arguments to `intro_main`), calls the function, then passes its return value to `SYS_exit`; `entry_smol.asm` adapts the entry for smol's runtime resolver.
- `intro_main.c` currently handles the platform: `XOpenDisplay` -> `glXChooseFBConfig` -> `XCreateWindow` (1024x640) -> `glXCreateContextAttribsARB` (GL 4.1 core) -> `XPending`/`XNextEvent` loop, rendering, `glXSwapBuffers`, and an approximately 16 ms `nanosleep`.
- X/GL types and external prototypes are declared in the project-owned `include/xlibfwd.h` and `include/glfwd.h`; no system-library headers are included.
- `src/c/minicrt/syscall.c` implements `sys_write` and `sys_sleep_ms` using inline-assembly Linux syscalls (`rax=1` and `rax=35`, respectively); declarations are in `include/minicrt/syscall.h`.

## Event Model

The event loop uses `XPending()` to read available events when Xlib's local queue is empty, then `XNextEvent()` to remove each queued event. Do not assume that `XCheck*` functions only inspect the existing local queue; their behavior may read from the connection. Handlers cover KeyPress (Escape/`q`) and `ClientMessage` (`WM_PROTOCOLS`/`WM_DELETE_WINDOW`) for normal shutdown, `UnmapNotify`/`DestroyNotify` for compositor/window-manager shutdown, and `ConfigureNotify` for viewport resizing.

## ELF Layout (`link.ld`)

The intro layout places allocatable sections together in one `PT_LOAD` segment with RWX permissions. This avoids page padding between separate loadable segments, at the cost of a writable-and-executable mapping. The current executable has additional program headers, including `PT_INTERP` and `PT_DYNAMIC`; “single `PT_LOAD`” does not mean it has only one program header.

Key invariants (violations may cause startup failure; see “Linker Pitfalls” in `AGENTS.md`):

- The `PT_LOAD` has `p_offset=0` and covers the ELF and program headers.
- For each `PT_LOAD`, `p_offset` and `p_vaddr` are congruent modulo the system page size (4 KiB on the tested system).
- The linker generates program headers; the linker script controls section placement.

Unpacked MinSizeRel executable composition (5,568 bytes):

| Component | Bytes |
|---|---:|
| Project code (`.text`) | 1,218 |
| Project data (`.rodata`, log strings) | 448 |
| PLT + `.rela.plt` + `.got.plt` | 1,240 |
| `.dynsym` + `.dynstr` + `.gnu.hash` + `.dynamic` + `.interp` | 1,339 |
| ELF/program headers, section-name table, and remaining layout overhead | 1,323 |

There are 25 imported symbols from X11/GLX/GL; the executable has no direct libc symbol imports.

## Size and Packer

The project provides three build variants: `gnu_ld_script` (GNU ld with the custom `link.ld`), `std_upx` (standard GNU ELF followed by UPX), and `smol` (custom ELF/runtime resolver). Compare their MinSizeRel outputs with `stat -c%s`; the UPX output is `std_upx.upx`, while `smol` writes `smol/intro.smol`. These formats have different loader and debugging trade-offs; see the repository README for build commands.

## Tests

Interactive smoke tests use `xdotool` (see `AGENTS.md`) and require an X11 display. CI requires an X server such as Xvfb and a usable GL implementation; headless GL can use Mesa llvmpipe when configured.
