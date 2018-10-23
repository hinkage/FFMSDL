//other code  
#include "stdafx.h"
#include "log.h"
#include <string>

std::string filter = "";

void _stdcall logd(const char *format, ...){
	char buf[4096], *p = buf;	va_list args;
	va_start(args, format);
	p += _vsnprintf(p, sizeof buf - 1, format, args);
	va_end(args);

	logd(buf);
}

void logd(char* logStr)
{
	FILE *fp;
	int len = strlen(logStr);
	fp = fopen("log.txt", "a+");
	if (fp == NULL) return;
	logStr[len] = '\n';//添加换行符在这里，就不用在其它地方重复添加了
	logStr[len + 1] = '\0';

	if (false == filter.empty()) {
		if (((std::string)logStr).find(filter) == -1) {
			fclose(fp);
			return;
		}
	}

	fwrite(logStr, strlen(logStr), 1, fp);
	OutputDebugStringA(logStr);
	
    fclose(fp);  
}

void logc(char* ft){
	filter = ft;
	FILE *fp;   
    fp = fopen("log.txt","w");
	fclose(fp);
}

void logc() {
	FILE *fp;
	fp = fopen("log.txt", "w");
	fclose(fp);
}
