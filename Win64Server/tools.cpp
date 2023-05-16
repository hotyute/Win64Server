#include "tools.h"

#define _USE_MATH_DEFINES
#include <math.h>

std::vector<std::string> &split(const std::string &str, const std::string &delimiters, std::vector<std::string> &elems) {
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		elems.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, const std::string &delim) {
	std::vector<std::string> elems;
	return split(s, delim, elems);
}

std::wstring s2ws(const std::string& s) {
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

SIZE getBitTextLength(HDC hDC, HFONT font, std::string text) {
	HFONT oldfont = (HFONT)SelectObject(hDC, font);
	SIZE extent;
	std::wstring wStr = s2ws(text);
	GetTextExtentPoint32(hDC, wStr.c_str(), text.length(), &extent );
	//std::cout << ((extent.cx) / 2) << std::endl;
	SelectObject(hDC, oldfont);
	return extent;
}

// trim from start
std::string ltrim(std::string s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c); }));
	return s;
}

// trim from end
std::string rtrim(std::string s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) {return !std::isspace(c); }).base(), s.end());
	return s;
}

// trim from both ends
std::string trim(std::string s) {
	return ltrim(rtrim(s));
}

int binarySearch1(double sortedArray[], int first, int last, double key) {
	// function:
	//   Searches sortedArray[first]..sortedArray[last] for key.  
	// returns: index of the matching element if it finds key, 
	//         otherwise  -(index where it could be inserted)-1.
	// parameters:
	//   sortedArray in  array of sorted (ascending) values.
	//   first, last in  lower and upper subscript bounds
	//   key         in  value to search for.
	// returns:
	//   index of key, or -insertion_position -1 if key is not 
	//                 in the array. This value can easily be
	//                 transformed into the position to insert it.

	while (first <= last) {
		int mid = (first + last) / 2;  // compute mid point.
		if (key > sortedArray[mid]) 
			first = mid + 1;  // repeat search in top half.
		else if (key < sortedArray[mid]) 
			last = mid - 1; // repeat search in bottom half.
		else
			return mid;     // found it. return position /////
	}
	return -(first + 1);   // failed to find key
}

int binarySearch(double sortedArray[], int length, double key) {
	return binarySearch1(sortedArray, 0, length, key);
}

bool pnpoly(int nvert, int *vertx, int *verty, int testx, int testy ) {
	bool c = false;
	int i, j;
	for(i = 0, j = nvert-1; i < nvert; j = i++) {
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i])) {
				c = !c;
		}
	}
	return c;
}

long long doubleToRawBits(double x) {
	long long bits;
	memcpy(&bits, &x, sizeof bits);
	return bits;
}

double longToRawBits(long long x) {
	double bits;
	memcpy(&bits, &x, sizeof bits);
	return bits;
}

char *s2ca1(const std::string &s) {
	char* res = new char[s.size()+1];
	strncpy_s(res, s.size()+1, s.c_str(), s.size()+1);
	return res;
}

void wordWrap(std::vector<std::string>& dest, const char* buffer, size_t maxlength, int indent) {
	size_t count, buflen;
	const char *ptr, *endptr;
	count = 0;
	buflen = strlen(buffer);
	do {
		ptr = buffer + count;

		/* don't set endptr beyond the end of the buffer */

		if (ptr - buffer + maxlength <= buflen)
			endptr = ptr + maxlength;
		else
			endptr = buffer + buflen;
		/* back up EOL to a null terminator or space */

		while (*(endptr) && !isspace(*(endptr)) )
			endptr--;
		const int size = (endptr - ptr);
		char* out;
		out = (char*) malloc (size);
		if (out) {
			strncpy(out, ptr, size);
			out[size] = '\0';
			rtrim(out);
			dest.push_back(out);
			count += size;
		}
//TODO FIx bug where word is longer than max length (word as in no spaces)
	} while (*endptr);
}

double dist(double lat1, double lon1, double lat2, double lon2) {
	double dist, dlon = lon2 - lon1;
	lat1 *= M_PI / 180.0;
	lat2 *= M_PI / 180.0;
	dlon *= M_PI / 180.0;
	dist = (sin(lat1) * sin(lat2)) + (cos(lat1) * cos(lat2) * cos(dlon));
	if (dist > 1.0) dist = 1.0;
	dist = acos(dist) * 60 * 180 / M_PI;
	return dist;
}

bool is_digits(const std::string& str)
{
	return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}
