#pragma once


//dynamic library import/export definitions.
#ifdef YAPT_WINDOWS
	#ifdef __GNUC__
		#define DLL_EXPORT __attribute__ ((dllexport))
		#define DLL_IMPORT __attribute__ ((dllimport))
	#else
		#define DLL_EXPORT __declspec(dllexport)
		#define DLL_IMPORT __declspec(dllimport)
	#endif 
#else
	#define DLL_IMPORT 
	#define DLL_EXPORT
#endif



