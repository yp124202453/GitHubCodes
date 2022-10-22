#pragma once


#ifdef _USRDLL

#define CppProject_Export_Ex __declspec(dllexport)

#else

#define CppProject_Export_Ex __declspec(dllimport)

#endif

extern "C" CppProject_Export_Ex int Add(int numberA, int numberB);

extern "C" CppProject_Export_Ex int Sub(int numberA, int numberB);