
#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void my_mkdir(const char* path)
{
	CreateDirectory(path, nullptr);
}

#else // WIN32

#include <sys/stat.h>
#include <sys/types.h>

void my_mkdir(const char* path)
{
	mkdir(path, 0777);
}

#endif // WIN32
