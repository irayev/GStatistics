#define _WIN32_WINNT 0x0600
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <vector>
#include "Utilities.h"
#include "SQLiteQueue.h"
#include "EventManager.h"
#include "GCore.h" 

#pragma comment(lib, "winhttp.lib")

// Глобальный объект для работы с очередью
SQLiteQueue g_queue;

// Экспортируемая функция для установки callback
extern "C" __declspec(dllexport) void __stdcall SetEventCallback(EventCallback callback)
{
    EventManager::SetCallback(callback);
}

// Надёжное безопасное копирование из MQL4 string
int SendRequestInternalSafe(const wchar_t* serverUrlPtr, const wchar_t* jsonBodyPtr)
{
    if (!serverUrlPtr || !jsonBodyPtr) return 1;

    size_t urlLen = std::min(wcslen(serverUrlPtr), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBodyPtr), size_t(8192));

    std::wstring serverUrl(serverUrlPtr, urlLen);
    std::wstring jsonBodyW(jsonBodyPtr, jsonLen);

    std::string bodyUtf8 = WideToUtf8(jsonBodyW.c_str());
    return SendRequestInternal(serverUrl, bodyUtf8);
}

std::wstring SendRequestInternalSafeResponse(const wchar_t* serverUrlPtr, const wchar_t* jsonBodyPtr)
{
    if (!serverUrlPtr || !jsonBodyPtr) return L"ERROR: Invalid parameters";

    size_t urlLen = std::min(wcslen(serverUrlPtr), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBodyPtr), size_t(8192));

    std::wstring serverUrl(serverUrlPtr, urlLen);
    std::wstring jsonBodyW(jsonBodyPtr, jsonLen);

    std::string bodyUtf8 = WideToUtf8(jsonBodyW.c_str());
    std::string responseUtf8 = SendRequestInternalResponse(serverUrl, bodyUtf8);

    return Utf8ToWide(responseUtf8.c_str());
}

// Функция для обработки очереди в фоновом потоке
DWORD WINAPI ProcessQueueThread(LPVOID lpParam) {
    EventManager::SendEvent(L"QUEUE_START", L"Начало обработки очереди");

    std::vector<QueueItem> items = g_queue.GetPendingItems(50);
    int processed = 0;
    int successful = 0;

    EventManager::SendEvent(L"QUEUE_STATUS", (L"Найдено " + std::to_wstring(items.size()) + L" записей").c_str());

    for (const auto& item : items) {
        processed++;
        if (g_queue.ProcessQueueItem(item)) {
            successful++;
            g_queue.RemoveFromQueue(item.id);
            EventManager::SendEvent(L"REQUEST_SUCCESS", (L"Успешно отправлен запрос ID: " + std::to_wstring(item.id)).c_str());
        }
        else {
            EventManager::SendEvent(L"REQUEST_FAILED", (L"Ошибка отправки запроса ID: " + std::to_wstring(item.id)).c_str());
        }
        Sleep(100);
    }

    EventManager::SendEvent(L"QUEUE_COMPLETE",
        (L"Обработка завершена. Успешно: " + std::to_wstring(successful) +
            L", Всего: " + std::to_wstring(processed)).c_str());

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Экспортируемые функции с callback-событиями
///////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    EventManager::SendEvent(L"SEND_REQUEST_START", L"Начало отправки запроса");

    int result = SendRequestInternalSafe(serverUrl, jsonBody);

    if (result == 0) {
        EventManager::SendEvent(L"SEND_REQUEST_SUCCESS", L"Запрос успешно отправлен");
    }
    else {
        EventManager::SendEvent(L"SEND_REQUEST_FAILED", L"Ошибка отправки запроса");
    }

    return result;
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    static thread_local std::wstring responseBuffer;

    EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_START", L"Начало отправки запроса с ответом");

    responseBuffer = SendRequestInternalSafeResponse(serverUrl, jsonBody);

    if (responseBuffer.find(L"ERROR:") == 0) {
        EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_FAILED", responseBuffer.c_str());
    }
    else {
        EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_SUCCESS", L"Запрос с ответом успешно обработан");
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall SendHttpRequestQueue(
    const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse)
{
    if (!serverUrl || !jsonBody) {
        EventManager::SendEvent(L"QUEUE_ADD_FAILED", L"Неверные параметры");
        return 1;
    }

    size_t urlLen = std::min(wcslen(serverUrl), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBody), size_t(8192));

    std::wstring serverUrlW(serverUrl, urlLen);
    std::wstring jsonBodyW(jsonBody, jsonLen);
    std::string jsonBodyUtf8 = WideToUtf8(jsonBodyW.c_str());

    bool result = g_queue.AddToQueue(serverUrlW, jsonBodyUtf8, expectResponse);

    if (result) {
        std::wstring message = expectResponse ?
            L"Запрос добавлен в очередь с ожиданием ответа" :
            L"Запрос добавлен в очередь без ожидания ответа";
        EventManager::SendEvent(L"QUEUE_ADD_SUCCESS", message.c_str());
    }
    else {
        EventManager::SendEvent(L"QUEUE_ADD_FAILED", L"Ошибка добавления в очередь");
    }

    return result ? 0 : 1;
}

extern "C" __declspec(dllexport) int __stdcall ProcessHttpQueue()
{
    EventManager::SendEvent(L"PROCESS_QUEUE_START", L"Запуск обработки очереди в фоновом режиме");

    HANDLE hThread = CreateThread(NULL, 0, ProcessQueueThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        EventManager::SendEvent(L"PROCESS_QUEUE_SUCCESS", L"Фоновый поток запущен успешно");
        return 0;
    }
    else {
        EventManager::SendEvent(L"PROCESS_QUEUE_FAILED", L"Ошибка запуска фонового потока");
        return 1;
    }
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall GetHttpResponse(
    const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    if (!serverUrl || !jsonBody) {
        EventManager::SendEvent(L"GET_RESPONSE_FAILED", L"Неверные параметры");
        return L"ERROR: Invalid parameters";
    }

    static thread_local std::wstring responseBuffer;

    size_t urlLen = std::min(wcslen(serverUrl), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBody), size_t(8192));

    std::wstring serverUrlW(serverUrl, urlLen);
    std::wstring jsonBodyW(jsonBody, jsonLen);
    std::string jsonBodyUtf8 = WideToUtf8(jsonBodyW.c_str());

    // Получаем и удаляем ответ
    std::string response = g_queue.GetAndRemoveResponse(serverUrlW, jsonBodyUtf8);

    if (!response.empty()) {
        responseBuffer = Utf8ToWide(response.c_str());
        EventManager::SendEvent(L"GET_RESPONSE_SUCCESS", L"Ответ успешно получен из базы");
    }
    else {
        responseBuffer = L"ERROR: No response found";
        EventManager::SendEvent(L"GET_RESPONSE_NOT_FOUND", L"Ответ не найден в базе");
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses)
{
    EventManager::SendEvent(L"CLEAN_START",
        (L"Начало очистки записей старше " + std::to_wstring(hoursOld) + L" часов").c_str());

    int result = g_queue.CleanOldItems(hoursOld, cleanResponses);

    EventManager::SendEvent(L"CLEAN_COMPLETE",
        (L"Очистка завершена. Удалено записей: " + std::to_wstring(result)).c_str());

    return result;
}

extern "C" __declspec(dllexport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses)
{
    int result = g_queue.GetOldItemsCount(hoursOld, checkResponses);

    std::wstring type = checkResponses ? L"ответов" : L"записей в очереди";
    EventManager::SendEvent(L"COUNT_RESULT",
        (L"Найдено " + std::to_wstring(result) + L" " + type +
            L" старше " + std::to_wstring(hoursOld) + L" часов").c_str());

    return result;
}

// Точка входа DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        EventManager::SendEvent(L"DLL_LOADED", L"Библиотека GCore загружена");
        break;
    case DLL_PROCESS_DETACH:
        EventManager::SendEvent(L"DLL_UNLOADED", L"Библиотека GCore выгружена");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}