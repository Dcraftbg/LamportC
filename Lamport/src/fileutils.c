#include "fileutils.h"
#if _WIN32
#include <Windows.h>
#else // On linux it uses a very dumb implementation
#endif
char* read_whole_file(const char* path, size_t* _size) {
	char* result = NULL;
	FILE* f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "[ERROR] Could not open file %s, %s", path, strerror(errno));
		return NULL;
	}
	if (fseek(f, 0, SEEK_END)) {
		fprintf(stderr, "[ERROR] Could not seek to the end of the file: %s", strerror(errno));
		goto DEFER;
	}
	size_t size = ftell(f);
	if (_size) *_size = size;
	if (fseek(f, 0, SEEK_SET)) {
		fprintf(stderr, "[ERROR] Could not seek to the start of the file: %s", strerror(errno));
		goto DEFER;
	}
	result = (char*)malloc(size + 1);
	if (!result) {
		fprintf(stderr, "[ERROR] Could not malloc buffer!");
		goto DEFER;
	}
	size_t byteCount = fread(result, 1, size, f);
	if (ferror(f)) {
		free(result);
		result = NULL;
		fprintf(stderr, "[ERROR] Could not read from file %s", strerror(errno));
		goto DEFER;
	}
	if (byteCount != size) {
		free(result);
		result = NULL;
		fprintf(stderr, "Could not read whole file at once");
		goto DEFER;
	}
	result[size] = '\0';
DEFER:
	fclose(f);
	return result;
}
int write_whole_file(const char* path, char* buf, size_t size) {
   int result=0;
   FILE* f = fopen(path, "wb");
   if(!f) return 1;
   while(size) {
        size_t s=fwrite(buf, 1, size, f);
        buf+=s;
        size-=s;
        if(result = ferror(f)) goto DEFER;
   }
DEFER:
   fclose(f);
   return result;
}

bool file_exists(const char* path)
{
#if _WIN32
	DWORD attrib = GetFileAttributesA(path);
	return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	struct stat statbuf;
	return stat(path, &statbuf) >= 0;
#endif

}
