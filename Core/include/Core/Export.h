#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

#ifdef _WIN32
#ifdef NOVA_BUILD_DLL
#define NOVA_API __declspec(dllexport)
#else
#define NOVA_API __declspec(dllimport)
#endif
#else
#define NOVA_API __attribute__((visibility("default")))
#endif