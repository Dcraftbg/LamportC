#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define defer_ret(x) do {result=(x);goto DEFER;} while(0)
// NOTE: Reads entire file WITH A NULL TERMINATOR.
// This is just useful utility because of how C works.
// If you want something that saves you the one byte of memory, use another language like C++ or Rust idc.
char* read_whole_file(const char* path, size_t* _size);
int write_whole_file(const char* path, char* buf, size_t size);
bool file_exists(const char* path);