#ifndef __log_H__
#define __log_H__
//logÊä³ö;  
#include <stdio.h>
#include <string>

extern std::string filter;
void logd(char* logStr);
void _stdcall logd(const char *format, ...);
void logc(char* ft);
void logc();

#endif // !__log_H__
