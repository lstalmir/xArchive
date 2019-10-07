#pragma once
#ifdef XARCHIVE_EXPORT
#define XARCHIVE_API __declspec(dllexport)
#else
#define XARCHIVE_API __declspec(dllimport)
#endif
