// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#ifdef SCOPECONTROL_EXPORTS
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport) 
#endif

#define _WIN32_WINNT 0x0501 //stops a compiler warning

#define LVEXP extern "C" //stops name mangling so labview plays nice with running the functions

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// TODO: reference additional headers your program requires here



//Own defs
#define SCOPECONTROL_EXPORTS