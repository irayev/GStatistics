#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

std::wstring ToWide(const char* str) {
    if (!str) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], len);
    if (!wstr.empty() && wstr.back() == 0) wstr.pop_back();
    return wstr;
}

extern "C" __declspec(dllexport) int __stdcall SendHttpRequest(
    const char* serverUrl,   // полный адрес
    const char* jsonBody)    // JSON
{
    std::wstring wServerUrl = ToWide(serverUrl);
    std::string jsonData(jsonBody);

    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256];
    wchar_t urlPath[1024];
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = _countof(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(wServerUrl.c_str(), 0, 0, &urlComp)) {
        return 1;
    }

    HINTERNET hSession = WinHttpOpen(L"MQL4 Client",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        NULL, NULL, 0);
    if (!hSession) return 1;

    HINTERNET hConnect = WinHttpConnect(hSession,
        urlComp.lpszHostName,
        urlComp.nPort,
        0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return 1;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        L"POST",
        urlComp.lpszUrlPath,
        NULL, NULL, NULL, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    int result = 1;
    if (WinHttpSendRequest(hRequest,
        headers, -1,
        (LPVOID)jsonData.c_str(),
        jsonData.length(),
        jsonData.length(), 0))
    {
        if (WinHttpReceiveResponse(hRequest, NULL))
        {
            DWORD statusCode = 0;
            DWORD statusSize = sizeof(statusCode);

            if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL, &statusCode, &statusSize, NULL))
            {
                if (statusCode == 200)
                    result = 0;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}