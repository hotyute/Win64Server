#ifndef TOOLS_H
#define TOOLS_H

#include <iostream>
#include <algorithm> 
#include <functional>
#include <cctype>
#include <locale>
#include <windows.h>
#include <vector>
#include <sstream>

extern std::wstring s2ws(const std::string& s);

extern char *s2ca1(const std::string &s);

extern SIZE getBitTextLength(HDC, HFONT, std::string);

extern std::string ltrim(std::string s);

extern std::string rtrim(std::string s);

extern std::string trim(std::string s);

extern std::vector<std::string> split(const std::string&, const std::string&);

extern int binarySearch(double a[], int length, double key);

extern bool pnpoly(int nvert, int *vertx, int *verty, int testx, int testy);

extern long long doubleToRawBits(double x);

double longToRawBits(long long);

extern void wordWrap(std::vector<std::string>& dest, const char* buffer, size_t maxlength, int indent);

#endif
