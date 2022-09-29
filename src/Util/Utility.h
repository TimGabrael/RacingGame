#pragma once
#define _DEBUGGING

#ifdef _DEBUGGING
#include <stdio.h>
#define LOG(msg, ...) printf(msg, __VA_ARGS__)
#else
#define LOG(msg, ...) 
#endif

#define RGBA(r,g,b,a) (a << 24) | (b << 16) | (g << 8) | r