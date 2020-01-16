#include <EASTL/allocator.h>

#include <cstdlib>

void* operator new[](size_t size, const char* name, int flags,
	unsigned debug_flags, const char* file, int line)
{
	(void)name;
	(void)flags;
	(void)debug_flags;
	(void)file;
	(void)line;

	return std::malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignment_offset,
	const char* name, int flags, unsigned debug_flags, const char* file, int line)
{
	(void)alignment;
	(void)alignment_offset;
	(void)name;
	(void)flags;
	(void)debug_flags;
	(void)file;
	(void)line;

	return std::malloc(size);
}

// FIXME: probably need operator new and delete with more malloc/free
