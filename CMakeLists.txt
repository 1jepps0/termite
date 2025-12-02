cmake_minimum_required(VERSION 3.15)

project(Termite)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# source files 
set(SOURCES
    src/app.c
    src/main.c
    src/text.c
    src/shader.c
    src/glad.c
	src/window.c
	src/pty_wrap.c
	src/esc_seq.c
    src/terminal.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/include
)

# link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/lib/libglfw3.a
	${CMAKE_SOURCE_DIR}/lib/libfreetype.a
	m   	  # math lib 
	png       # libpng
	z         # zlib
	bz2       # bzip2
	util      # for pty
)

