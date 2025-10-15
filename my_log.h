#pragma once
#include <debugapi.h>
#include <cstdarg>
#include <stdio.h>

#ifdef _DEBUG
inline void MyLog(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
}
#else
inline void MyLog(const char* fmt, ...) { (void)fmt; }
#endif