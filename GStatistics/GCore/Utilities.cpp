#include "Utilities.h"
#include <winhttp.h>
#include <vector>

#pragma comment(lib, "winhttp.lib")

// Конвертация UTF-16 → UTF-8
std::string WideToUtf8(const wchar_t* wstr)
{
    if (!wstr) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    std::string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size_needed, NULL, NULL);
    return str;
}

// Конвертация UTF-8 → UTF-16
std::wstring Utf8ToWide(const char* str)
{
    if (!str) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (size_needed <= 0) return L"";
    std::wstring wstr(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], size_needed);
    return wstr;
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

std::string SendRequestInternalResponse(const std::wstring& serverUrl, const std::string& jsonBody)
{
    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256]{};
    wchar_t urlPath[1024]{};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = _countof(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(serverUrl.c_str(), 0, 0, &urlComp)) return "ERROR: Failed to parse URL";

    HINTERNET hSession = WinHttpOpen(L"GStatistics/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "ERROR: Failed to open session";

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return "ERROR: Failed to connect"; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, NULL, NULL, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return "ERROR: Failed to create request"; }

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(hRequest, headers, -1,
        (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.size(), (DWORD)jsonBody.size(), 0);

    if (!sent) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "ERROR: Failed to send request";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "ERROR: Failed to receive response";
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &size, NULL);

    std::string response;
    DWORD bytesAvailable = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            response = "ERROR: Failed to query available data";
            break;
        }

        if (bytesAvailable == 0) break;

        std::vector<char> buffer(bytesAvailable + 1);
        DWORD bytesRead = 0;

        if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            response = "ERROR: Failed to read data";
            break;
        }

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response.append(buffer.data(), bytesRead);
        }

    } while (bytesAvailable > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (statusCode != 200) {
        return "ERROR: HTTP " + std::to_string(statusCode) + " - " + response;
    }

    return response;
}