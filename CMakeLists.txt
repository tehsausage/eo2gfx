cmake_minimum_required(VERSION 3.1)
project(eo2gfx C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")
endif()

# ---------
#  Options
# ---------

option(EO2GFX_USE_PRECOMPILED_HEADERS "Uses a precompiled header to speed up compilation." ON)

option(EO2GFX_GEN_PRECOMPILED_HEADERS "Generate a precompiled header automatically. Requires a shell with basic binutils including grep and awk." ON)

# --------------
#  Source files
# --------------

include(SourceFileList)

set(gfxdump_SOURCE_FILES ${gfxdump_ALL_SOURCE_FILES})
set(eoatlas_SOURCE_FILES ${eoatlas_ALL_SOURCE_FILES})

# ---------
#  Outputs
# ---------

add_executable(gfxdump
	${gfxdump_SOURCE_FILES}
)

add_executable(eoatlas
	${eoatlas_SOURCE_FILES}
)

install(TARGETS gfxdump RUNTIME DESTINATION .)
install(TARGETS eoatlas RUNTIME DESTINATION .)

# -----------
#  Libraries
# -----------

include_directories(src)

find_package(PNG REQUIRED)

target_include_directories(gfxdump SYSTEM PRIVATE ${PNG_INCLUDE_DIR})
target_link_libraries(gfxdump ${PNG_LIBRARY})

target_include_directories(eoatlas SYSTEM PRIVATE ${PNG_INCLUDE_DIR})
target_link_libraries(eoatlas ${PNG_LIBRARY})

# ---------------------
#  Precompiled Headers
# ---------------------

include(PCH)
include(PCHAutoGen)

if(EO2GFX_USE_PRECOMPILED_HEADERS)
	if(EO2GFX_GEN_PRECOMPILED_HEADERS)
		autogen_pch(gfxdump-pch-autogen ${CMAKE_CACHEFILE_DIR}/gfxdump-pch.h ${gfxdump_ALL_SOURCE_FILES})
		autogen_pch(eoatlas-pch-autogen ${CMAKE_CACHEFILE_DIR}/eoatlas-pch.h ${eoatlas_ALL_SOURCE_FILES})
	else()
		autogen_pch_copy(gfxdump-pch-autogen ${CMAKE_CACHEFILE_DIR}/gfxdump-pch.h src/gfxdump/stdafx.h)
		autogen_pch_copy(eoatlas-pch-autogen ${CMAKE_CACHEFILE_DIR}/eoatlas-pch.h src/eoatlas/stdafx.h)
	endif()

	# ---

	compile_pch(gfxdump gfxdump-pch ${CMAKE_CACHEFILE_DIR}/gfxdump-pch.h)

	add_dependencies(gfxdump-pch-autogen gfxdump-pch)
	add_dependencies(gfxdump gfxdump-pch)

	separate_arguments(gfxdump-pch_INCLUDE_FLAG UNIX_COMMAND ${gfxdump-pch_INCLUDE_FLAG})
	target_compile_options(gfxdump PRIVATE ${gfxdump-pch_INCLUDE_FLAG})

	# ---

	compile_pch(eoatlas eoatlas-pch ${CMAKE_CACHEFILE_DIR}/eoatlas-pch.h)

	add_dependencies(eoatlas-pch-autogen eoatlas-pch)
	add_dependencies(eoatlas eoatlas-pch)

	separate_arguments(eoatlas-pch_INCLUDE_FLAG UNIX_COMMAND ${eoatlas-pch_INCLUDE_FLAG})
	target_compile_options(eoatlas PRIVATE ${eoatlas-pch_INCLUDE_FLAG})
endif()

# -------
#  Tests
# -------

#if (EO2GFX_WANT_TESTS)
#	enable_testing()
#	add_subdirectory(tests)
#endif()

