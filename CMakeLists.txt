cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE PATH "Prefix prepended to install directories.")
project(RandomStuff CXX ISPC)

# Visualizing the giftwrap convex hull algorithm
add_executable(giftwrap giftwrap.cc)
target_link_libraries(giftwrap SDL2 SDL2_gfx)

# Mandelbrot set generation
set(CMAKE_CXX_FLAGS "-march=native")
#set(CMAKE_ISPC_FLAGS "--target=avx2")
add_executable(mandelbrot mandelbrot.cc mandelbrot.ispc)
target_link_libraries(mandelbrot SDL2)

# An implementation of diff with visualization of the steps
add_executable(my_diff my_diff.cc)
target_link_libraries(my_diff curses)
target_compile_options(my_diff PUBLIC -fsanitize=address -fsanitize=undefined -O0)
target_link_options(my_diff PUBLIC -fsanitize=address -fsanitize=undefined)
