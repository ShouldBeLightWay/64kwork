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
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS_DEBUG "-F dwarf -g")

set(INTRO_COMMON_COMPILE_OPTIONS
    -ffreestanding
    -fno-pie
    -fno-stack-protector
    -fno-asynchronous-unwind-tables
    -fno-builtin
    -fno-ident
    -fcf-protection=none
    -Wall -Wextra
)

add_library(intro_main OBJECT
    src/c/intro_main.c
    src/c/minicrt/syscall.c
)
target_compile_options(intro_main PRIVATE ${INTRO_COMMON_COMPILE_OPTIONS})
target_include_directories(intro_main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(intro_main PROPERTIES POSITION_INDEPENDENT_CODE OFF)

function(add_intro_target target entry_source)
    add_executable(${target} ${entry_source} $<TARGET_OBJECTS:intro_main>)
    target_link_libraries(${target} PRIVATE X11 GL)
    target_link_options(${target} PRIVATE
        -nostdlib
        -no-pie
        -Wl,--build-id=none
        -Wl,--gc-sections
        $<$<CONFIG:Release,MinSizeRel>:-Wl,-s>
    )
endfunction()

# Existing compact ELF layout. This executable is a linker baseline, not a packed release.
add_intro_target(gnu_ld_script src/asm/entry.asm)
target_link_options(gnu_ld_script PRIVATE
    -Wl,-e,_start
    -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/link.ld
    -Wl,-z,noseparate-code
    -Wl,-rpath-link,/usr/lib
)

# Standard GNU ELF layout, followed by UPX compression.
add_intro_target(std_upx_elf src/asm/entry.asm)
target_link_options(std_upx_elf PRIVATE -Wl,-e,_start)
find_program(UPX_EXECUTABLE NAMES upx)
if(UPX_EXECUTABLE)
    set(UPX_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/std_upx.upx")
    add_custom_command(OUTPUT "${UPX_OUTPUT}"
        COMMAND ${UPX_EXECUTABLE} --best --lzma --force-overwrite
            --output=${UPX_OUTPUT}
            $<TARGET_FILE:std_upx_elf>
        DEPENDS std_upx_elf $<TARGET_FILE:std_upx_elf>
        VERBATIM
    )
    add_custom_target(std_upx DEPENDS "${UPX_OUTPUT}")
    add_dependencies(std_upx std_upx_elf)
else()
    add_custom_target(std_upx DEPENDS std_upx_elf)
    message(STATUS "UPX not found: std_upx target will produce only the unpacked std_upx_elf")
endif()

# smol uses its own loader/startup code and requires a dedicated _start section.
set(SMOL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tools/smol" CACHE PATH "Path to the smol checkout")
set(SMOL_CC "${CMAKE_C_COMPILER}" CACHE FILEPATH "GCC compiler used by smol")
find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(SMOL_NASM_EXECUTABLE NAMES nasm)
file(GLOB_RECURSE SMOL_TOOL_FILES CONFIGURE_DEPENDS
    "${SMOL_ROOT}/*.py"
    "${SMOL_ROOT}/rt/*.asm"
    "${SMOL_ROOT}/ld/*.ld"
)

if(Python3_Interpreter_FOUND AND SMOL_NASM_EXECUTABLE
   AND EXISTS "${SMOL_ROOT}/smold.py" AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_library(smol_entry OBJECT src/asm/entry_smol.asm)
    set(SMOL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/smol/intro.smol")
    set(SMOL_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/smol")
    set(SMOL_LINKED_OUTPUT "${SMOL_WORK_DIR}/intro-linked")
    set(SMOL_MAIN_OBJECT "${SMOL_WORK_DIR}/intro_main.c.o")
    set(SMOL_SYSCALL_OBJECT "${SMOL_WORK_DIR}/syscall.c.o")
    set(SMOL_ENTRY_OBJECT "${SMOL_WORK_DIR}/entry_smol.o")

    add_custom_command(OUTPUT "${SMOL_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SMOL_WORK_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_OBJECTS:intro_main> "${SMOL_WORK_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_OBJECTS:smol_entry> "${SMOL_ENTRY_OBJECT}"
        COMMAND ${Python3_EXECUTABLE} "${SMOL_ROOT}/smold.py"
            -d
            --cc "${SMOL_CC}"
            --nasm "${SMOL_NASM_EXECUTABLE}"
            -L /usr/lib
            --smolrt "${SMOL_ROOT}/rt"
            --smolld "${SMOL_ROOT}/ld"
            -l X11
            -l GL
            -fuse-dnload-loader
            -fskip-zero-value
            -fuse-dt-debug
            "${SMOL_MAIN_OBJECT}"
            "${SMOL_SYSCALL_OBJECT}"
            "${SMOL_ENTRY_OBJECT}"
            "${SMOL_LINKED_OUTPUT}"
        COMMAND ${Python3_EXECUTABLE} "${SMOL_ROOT}/smoltrunc.py"
            "${SMOL_LINKED_OUTPUT}" "${SMOL_OUTPUT}"
        COMMAND chmod +x "${SMOL_OUTPUT}"
        DEPENDS intro_main smol_entry ${SMOL_TOOL_FILES}
        COMMENT "Linking executable with smol"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    add_custom_target(smol DEPENDS "${SMOL_OUTPUT}")
else()
    message(STATUS "smol target disabled: requires smol sources, GCC, Python 3, and NASM")
endif()

message(STATUS "Build variants: gnu_ld_script, smol (when available), std_upx")
