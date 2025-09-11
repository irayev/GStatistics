#ifndef GSTATISTICS_H
#define GSTATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

	// Основные функции отправки
	__declspec(dllimport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody);
	__declspec(dllimport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	// Функции работы с очередью
	__declspec(dllimport) int __stdcall SendHttpRequestQueue(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse);
	__declspec(dllimport) int __stdcall ProcessHttpQueue();
	__declspec(dllimport) const wchar_t* __stdcall GetHttpResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	// Функции управления базой
	__declspec(dllimport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses);
	__declspec(dllimport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses);

#ifdef __cplusplus
}
#endif

#endif // GSTATISTICS_H