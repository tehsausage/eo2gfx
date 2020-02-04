cmake_minimum_required(VERSION 3.8)
project(eo2gfx C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")
endif()

# --------------
#  Source files
# --------------

find_package(PNG REQUIRED)

add_library(eo2gfx_common STATIC
	src/cio/cio.cpp
	src/cio/cio.hpp
	src/dib_reader.cpp
	src/dib_reader.hpp
	src/dib_writer.cpp
	src/dib_writer.hpp
	src/int_pack.hpp
	src/my_cstrcat.cpp
	src/my_cstrcat.hpp
	src/my_mkdir.cpp
	src/my_mkdir.hpp
	src/pe_reader.cpp
	src/pe_reader.hpp
	src/png_reader.cpp
	src/png_reader.hpp
	src/png_writer.cpp
	src/png_writer.hpp
)

target_include_directories(eo2gfx_common PUBLIC src/)

target_include_directories(eo2gfx_common SYSTEM PUBLIC ${PNG_INCLUDE_DIR})
target_link_libraries(eo2gfx_common PUBLIC ${PNG_LIBRARY})

add_library(eo2gfx_sha256 STATIC
	src/sha256/sha256.c
	src/sha256/sha256.h
	src/sha256/sha256_to_hex.c
	src/sha256/sha256_to_hex.h
)

target_include_directories(eo2gfx_sha256 INTERFACE src/ PRIVATE src/sha256/)

# -----------

add_executable(gfxdump
	src/gfxdump/main.cpp
)

target_link_libraries(gfxdump eo2gfx_common)

install(TARGETS gfxdump RUNTIME DESTINATION .)

# -----------

add_executable(eoatlas
	src/eoatlas/main.cpp
)

target_link_libraries(eoatlas eo2gfx_common)
target_link_libraries(eoatlas eo2gfx_sha256)

install(TARGETS eoatlas RUNTIME DESTINATION .)

# -----------

add_executable(fmtid
	src/fmtid/main.cpp
)

target_link_libraries(fmtid eo2gfx_common)

install(TARGETS fmtid RUNTIME DESTINATION .)

# -----------

add_executable(bmpinfo
	src/bmpinfo/main.cpp
)

target_link_libraries(bmpinfo eo2gfx_common)

install(TARGETS bmpinfo RUNTIME DESTINATION .)

# -----------

add_executable(png2bmp
	src/png2bmp/main.cpp
)

target_link_libraries(png2bmp eo2gfx_common)

install(TARGETS png2bmp RUNTIME DESTINATION .)
