#define _WIN32_WINNT 0x0600
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <algorithm> // для std::min и std::max

#pragma comment(lib, "winhttp.lib")

// Конвертация UTF-16 → UTF-8
std::string WideToUtf8(const wchar_t* wstr)
{
    if (!wstr) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    std::string str(size_needed - 1, 0); // -1 чтобы не включать \0
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size_needed, NULL, NULL);
    return str;
}

// Внутренний POST через WinHTTP
int SendRequestInternal(const std::wstring& serverUrl, const std::string& jsonBody)
{
    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256]{};
    wchar_t urlPath[1024]{};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = _countof(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(serverUrl.c_str(), 0, 0, &urlComp)) return 1;

    HINTERNET hSession = WinHttpOpen(L"GStatistics/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return 1;

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return 1; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, NULL, NULL, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return 1; }

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(hRequest, headers, -1,
        (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.size(), (DWORD)jsonBody.size(), 0);

    if (!sent || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return 1;
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &size, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return (statusCode == 200) ? 0 : 1;
}

// Надёжное безопасное копирование из MQL4 string
int SendRequestInternalSafe(const wchar_t* serverUrlPtr, const wchar_t* jsonBodyPtr)
{
    if (!serverUrlPtr || !jsonBodyPtr) return 1;

    // безопасное ограничение длины
    size_t urlLen = std::min(wcslen(serverUrlPtr), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBodyPtr), size_t(8192));

    std::wstring serverUrl(serverUrlPtr, urlLen);
    std::wstring jsonBodyW(jsonBodyPtr, jsonLen);

    std::string bodyUtf8 = WideToUtf8(jsonBodyW.c_str());
    return SendRequestInternal(serverUrl, bodyUtf8);
}

///////////////////////////////////////////////////////////////////////////////
// Экспортируемые функции
///////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    return SendRequestInternalSafe(serverUrl, jsonBody);
}

// Точка входа DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}