#pragma once

#include <string>
#include <winnls.h>
namespace YAPT
{
	//String Utilities
	inline void stringToWString(const std::string& str, std::wstring& out)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
		if (size == 0) return;
		out.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &out[0], size);
	}

	inline void stringToWString(const char* str, std::wstring& out)
	{
		size_t len = strlen(str);
		int size = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, NULL, 0);
		if (size == 0) return;
		out.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, str, (int)len, &out[0], size);
	}
}