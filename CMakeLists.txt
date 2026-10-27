cmake_minimum_required(VERSION 3.20)

project(intro64k
    VERSION 0.1.0
    DESCRIPTION "64k intro framework (freestanding, no CRT)"
    LANGUAGES C ASM_NASM
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This project only supports Linux.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# NASM: 64-bit ELF object, debug info for gdb
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS_DEBUG "-F dwarf -g")

add_executable(intro)

target_sources(intro PRIVATE
    src/asm/entry.asm
    src/c/intro_main.c
)

# Freestanding: no libc, no CRT, no stack protector, no PIE
target_compile_options(intro PRIVATE
    $<$<COMPILE_LANGUAGE:C>:
        -ffreestanding
        -fno-pie
        -fno-stack-protector
        -fno-asynchronous-unwind-tables
        -fno-builtin
        -fno-ident
        -fcf-protection=none
        -Wall -Wextra
    >
)

target_link_options(intro PRIVATE
    -nostdlib
    -no-pie
    -Wl,--build-id=none
    -Wl,--gc-sections
    -Wl,-e,_start
    # Intro-style layout: one PT_LOAD, no page padding (see link.ld)
    -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/link.ld
    -Wl,-z,noseparate-code
    -Wl,-rpath-link,/usr/lib
)

# System shared libs: referenced via PLT, cost nothing in the 64k body
target_link_libraries(intro PRIVATE X11 GL)

# Keep the binary small and section-padded
target_link_options(intro PRIVATE
    $<$<CONFIG:Release,MinSizeRel>:-Wl,-s>
)
