find_path(EASTL_INCLUDE_DIR
	NAMES EASTL/vector.h
)

find_library(EASTL_LIBRARY
	NAMES EASTL
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	EASTL DEFAULT_MSG
	EASTL_LIBRARY EASTL_INCLUDE_DIR
)

mark_as_advanced(
	EASTL_INCLUE_DIR
	EASTL_LIBRARY
)
