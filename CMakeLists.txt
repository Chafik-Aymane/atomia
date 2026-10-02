cmake_minimum_required(VERSION 3.10)
project(Atomia C)

# headers
include_directories(include)

# library files (.a / .dll.a)
link_directories(lib)

# source files
add_executable(Atomia src/main.c
    src/atom.c
    include/elements.h
    src/elements.c)

# Link Raylib and its dependencies
# For Windows (MinGW/GCC), Raylib needs these system libraries to work:
target_link_libraries(Atomia 
    raylib
    opengl32
    gdi32
    winmm
    
)

# Optional: Ensure the .dll is copied to the build folder so the .exe can run
add_custom_command(TARGET Atomia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:Atomia>/resources"
    COMMENT "Copying resources to build directory..."
)

# using bash :
# mkdir build && cd build
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake --build .