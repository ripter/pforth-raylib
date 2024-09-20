cmake_minimum_required(VERSION 3.10)

# Project name
project(TestPForthCCode)

# Set the C standard to C99 or whichever version you need
set(CMAKE_C_STANDARD 99)

# Add the parent directory to the include path for pforth.h
include_directories(..)

# Find Raylib using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)

# Add the include directories and link flags from pkg-config
include_directories(${RAYLIB_INCLUDE_DIRS})
link_directories(${RAYLIB_LIBRARY_DIRS})
add_definitions(${RAYLIB_CFLAGS_OTHER})

# Add your test source files
add_executable(test_stack stack_reset_test.c ../pf_core.c)

# Link against Raylib using pkg-config
target_link_libraries(test_stack ${RAYLIB_LIBRARIES})

# Optionally add a command to run the executable after build
add_custom_target(run_tests
    COMMAND test_stack
    DEPENDS test_stack
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
