# 64kwork Roadmap

## Phase 0: Skeleton and Baseline (Complete)

- [x] CMake + NASM, freestanding build (`-ffreestanding`, `-nostdlib`), and a custom `_start`.
- [x] Xlib window + OpenGL 4.1 core context (`glXCreateContextAttribsARB`), frame loop, and clean shutdown.
- [x] Intro-style ELF layout (`link.ld`, one `PT_LOAD` without inter-segment page padding), stripping, and removal of unused metadata (`.comment`, `.eh_frame`, symbol table). GNU ld MinSizeRel size: 5,568 bytes.
- [x] Project documentation and `AGENTS.md` with implementation rules and pitfalls.

Phase metric: reduced the baseline from 15,088 to 5,568 bytes without losing functionality.

## Phase 1: minicrt (Our Minimal Runtime)

Choose between a STATIC library and an OBJECT library based on tests and size measurements. An object library may avoid archive overhead during linking.

- [ ] Move syscall wrappers from `intro_main.c` into `src/c/minicrt/` (`sys_write`, `sys_read`, `nanosleep`, `mmap`, etc.).
- [ ] Add minimal project-named string/memory functions (`strlen`, `memcpy`, `memset`) and a small allocator, or use an `mmap`/`brk` arena if the intro's needs favor it.
- [ ] Add a small number-to-string formatter for debug output; later remove debug output from release builds to save `.rodata` space.
- [ ] Compare dynamic X11/GL imports through `ld.so` with a custom resolver using `link_map`/`DT_*` tables. Measure whether avoiding `ld.so` or reducing PLT/relocation overhead (estimated at about 1.2 KiB) is worthwhile; retain the dynamic loader otherwise. Use the same scene in both variants.
- [ ] Add a minimal test runner without libc (custom `main`, exit code equals test result) and test under Xvfb when available (`xorg-server-xvfb`).

Acceptance criterion: preserve current runtime functionality without exceeding the current baseline size. Document STATIC vs OBJECT measurements in `doc/minicrt.md`.

## Phase 2: Engine (OpenGL Engine)

Create separate documentation in `doc/engine.md` when this phase starts. The central goal is **procedural generation**: store minimal seeds and parameters instead of large assets.

- [ ] Build a GL 4.1 core shader pipeline (minimal VAO/VBO use, shaders embedded as strings, and a small helper around `glXGetProcAddress` for required entry points).
- [ ] Explore procedural geometry and textures (noise, L-systems, SDFs, or techniques selected for the scene).
- [ ] Track function-size measurements by comparing builds with `-ffunction-sections` and selectively disabled functions.
- [ ] Define the scene and timeline format (a small DSL or binary format), in coordination with the editor.
- [ ] Evaluate audio as an optional feature. Choose the synthesizer and output backend based on the engine's requirements; defer the design until `engine.md`.

## Phase 3: Editor (Scene Editor)

The editor is a separate executable and is not part of the intro's 64 KiB budget. It is currently intended to build on Linux.

- [ ] Build the UI with Nuklear (single-header, with no external dependencies).
- [ ] Reuse the engine's OpenGL rendering path (`src/engine/` should be shared by the intro and editor); the editor adds UI and tools.
- [ ] Provide a visual timeline for procedural-generation parameters and export to the Phase 2 scene format.

## Phase 4: Packing and Release Cycle

- [ ] Benchmark UPX (`upx --best --lzma`) as a baseline.
- [ ] Investigate a custom ELF/assembly packer. UPX includes a decompressor; a custom one may be smaller. The single-`PT_LOAD` layout may help.
- [ ] Add a CI size gate that fails when the packed artifact exceeds a configured limit (for example, 65,536 bytes).

## Cross-Phase Rules

1. Estimate byte cost before adding a feature and record a `stat` measurement after the change.
2. Use dynamic libraries in the intro (except for the planned minicrt); their code stays outside the executable, but dynamic symbols, relocations, PLT/GOT entries, and strings still consume space in it.
3. Keep the runtime smoke test passing for each phase: window creation, GL 4.1 context, and shutdown via Escape or the window close control.
4. Put new modules under `src/<module>/` and connect them through CMake targets; freestanding flags should be inherited automatically.
