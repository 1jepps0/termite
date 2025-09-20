cmake_minimum_required(VERSION 3.15)

project(term)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# enable warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif (MSVC)
    add_compile_options(/W4)
endif()

# find dependencies 
find_package(PkgConfig REQUIRED)

# pkg_check_modules(GLFW3 REQUIRED glfw3)
# pkg_check_modules(FREETYPE2 REQUIRED freetype2)

# source files 
set(SOURCES
    src/main.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLFW3_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
)

# link libraries
#target_link_libraries(${PROJECT_NAME} PRIVATE
#    ${GLFW3_LIBRARIES}
#    ${FREETYPE2_LIBRARIES}
#    m   # math lib (needed on Linux for some OpenGL/FreeType calls)
#)

