#pragma once

#ifdef GSTATISTICS_EXPORTS
#define GST_API extern "C" __declspec(dllexport)
#else
#define GST_API extern "C" __declspec(dllimport)
#endif

// Экспортируемая функция
GST_API int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody);
