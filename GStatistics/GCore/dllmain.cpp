#define _WIN32_WINNT 0x0600
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include "Utilities.h"
#include "SQLiteQueue.h"
#include "EventManager.h"
#include "GCore.h"

#pragma comment(lib, "winhttp.lib")

// Глобальный объект для работы с очередью
SQLiteQueue g_queue;

// Глобальные переменные для хранения событий
static std::vector<std::pair<std::wstring, std::wstring>> g_events;
static CRITICAL_SECTION g_eventsCs;

// -----------------------------------------------------------------------------
// Инициализация критической секции
// -----------------------------------------------------------------------------
void InitializeEventsSystem()
{
    InitializeCriticalSection(&g_eventsCs);
}

// -----------------------------------------------------------------------------
// Добавление события в очередь
// -----------------------------------------------------------------------------
void AddEventToQueue(const wchar_t* event_type, const wchar_t* data)
{
    EnterCriticalSection(&g_eventsCs);
    g_events.push_back(std::make_pair(event_type, data));
    LeaveCriticalSection(&g_eventsCs);
}

// -----------------------------------------------------------------------------
// Вспомогательная функция для управления событиями
// -----------------------------------------------------------------------------
inline void HandleEvent(const wchar_t* type, const wchar_t* data, bool useSendEvent, bool useQueueEvent)
{
    if (useSendEvent)
        EventManager::SendEvent(type, data);
    if (useQueueEvent)
        AddEventToQueue(type, data);
}

// -----------------------------------------------------------------------------
// Экспортируемые функции для работы с событиями
// -----------------------------------------------------------------------------
extern "C" __declspec(dllexport) int __stdcall GetPendingEventCount()
{
    EnterCriticalSection(&g_eventsCs);
    int count = (int)g_events.size();
    LeaveCriticalSection(&g_eventsCs);
    return count;
}

extern "C" __declspec(dllexport) int __stdcall GetNextEvent(wchar_t* eventType, int typeSize, wchar_t* eventData, int dataSize)
{
    EnterCriticalSection(&g_eventsCs);

    if (g_events.empty())
    {
        LeaveCriticalSection(&g_eventsCs);
        return 0;
    }

    auto event = g_events.front();
    g_events.erase(g_events.begin());

    if (eventType && typeSize > 0)
        wcsncpy_s(eventType, typeSize, event.first.c_str(), _TRUNCATE);

    if (eventData && dataSize > 0)
        wcsncpy_s(eventData, dataSize, event.second.c_str(), _TRUNCATE);

    LeaveCriticalSection(&g_eventsCs);
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall ClearEvents()
{
    EnterCriticalSection(&g_eventsCs);
    g_events.clear();
    LeaveCriticalSection(&g_eventsCs);
    return 1;
}

extern "C" __declspec(dllexport) void __stdcall SetEventCallback(EventCallback callback)
{
    EventManager::SetCallback(callback);
}

// -----------------------------------------------------------------------------
// Внутренние безопасные функции
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Поток обработки очереди
// -----------------------------------------------------------------------------
DWORD WINAPI ProcessQueueThread(LPVOID lpParam) {
    HandleEvent(L"QUEUE_START", L"Начало обработки очереди", false, false);

    std::vector<QueueItem> items = g_queue.GetPendingItems(50);
    int processed = 0;
    int successful = 0;

    std::wstring statusMsg = L"Найдено " + std::to_wstring(items.size()) + L" записей";
    HandleEvent(L"QUEUE_STATUS", statusMsg.c_str(), false, false);

    for (const auto& item : items) {
        processed++;
        if (g_queue.ProcessQueueItem(item)) {
            successful++;
            g_queue.RemoveFromQueue(item.id);
            std::wstring successMsg = L"Успешно отправлен запрос ID: " + std::to_wstring(item.id);
            HandleEvent(L"REQUEST_SUCCESS", successMsg.c_str(), false, false);
        }
        else {
            std::wstring errorMsg = L"Ошибка отправки запроса ID: " + std::to_wstring(item.id);
            HandleEvent(L"REQUEST_FAILED", errorMsg.c_str(), false, false);
        }
        Sleep(100);
    }

    std::wstring completeMsg = L"Обработка завершена. Успешно: " + std::to_wstring(successful) + L", Всего: " + std::to_wstring(processed);
    HandleEvent(L"QUEUE_COMPLETE", completeMsg.c_str(), false, false);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Старые экспортируемые функции (оставлены без изменений)
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    HandleEvent(L"SEND_REQUEST_START", L"Начало отправки запроса", false, false);
    int result = SendRequestInternalSafe(serverUrl, jsonBody);

    if (result == 0)
        HandleEvent(L"SEND_REQUEST_SUCCESS", L"Запрос успешно отправлен", false, false);
    else
        HandleEvent(L"SEND_REQUEST_FAILED", L"Ошибка отправки запроса", false, false);

    return result;
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    static thread_local std::wstring responseBuffer;

    HandleEvent(L"SEND_REQUEST_RESPONSE_START", L"Начало отправки запроса с ответом", false, false);
    responseBuffer = SendRequestInternalSafeResponse(serverUrl, jsonBody);

    if (responseBuffer.find(L"ERROR:") == 0)
        HandleEvent(L"SEND_REQUEST_RESPONSE_FAILED", responseBuffer.c_str(), false, false);
    else
        HandleEvent(L"SEND_REQUEST_RESPONSE_SUCCESS", L"Запрос с ответом успешно обработан", false, false);

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall SendHttpRequestQueue(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse)
{
    if (!serverUrl || !jsonBody) {
        HandleEvent(L"QUEUE_ADD_FAILED", L"Неверные параметры", false, false);
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
        HandleEvent(L"QUEUE_ADD_SUCCESS", message.c_str(), false, false);
    }
    else {
        HandleEvent(L"QUEUE_ADD_FAILED", L"Ошибка добавления в очередь", false, false);
    }

    return result ? 0 : 1;
}

extern "C" __declspec(dllexport) int __stdcall ProcessHttpQueue()
{
    HandleEvent(L"PROCESS_QUEUE_START", L"Запуск обработки очереди в фоновом режиме", false, false);

    HANDLE hThread = CreateThread(NULL, 0, ProcessQueueThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        HandleEvent(L"PROCESS_QUEUE_SUCCESS", L"Фоновый поток запущен успешно", false, false);
        return 0;
    }
    else {
        HandleEvent(L"PROCESS_QUEUE_FAILED", L"Ошибка запуска фонового потока", false, false);
        return 1;
    }
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall GetHttpResponse(const wchar_t* serverUrl, const wchar_t* jsonBody)
{

    if (!serverUrl || !jsonBody) {
        HandleEvent(L"GET_RESPONSE_FAILED", L"Неверные параметры", false, false);
        return L"ERROR: Invalid parameters";
    }

    static thread_local std::wstring responseBuffer;

    size_t urlLen = std::min(wcslen(serverUrl), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBody), size_t(8192));

    std::wstring serverUrlW(serverUrl, urlLen);
    std::wstring jsonBodyW(jsonBody, jsonLen);
    std::string jsonBodyUtf8 = WideToUtf8(jsonBodyW.c_str());

    std::string response = g_queue.GetAndRemoveResponse(serverUrlW, jsonBodyUtf8);

    if (!response.empty()) {
        responseBuffer = Utf8ToWide(response.c_str());
        HandleEvent(L"GET_RESPONSE_SUCCESS", L"Ответ успешно получен из базы", false, false);
    }
    else {
        responseBuffer = L"ERROR: No response found";
        HandleEvent(L"GET_RESPONSE_NOT_FOUND", L"Ответ не найден в базе", false, false);
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses)
{
    std::wstring startMsg = L"Начало очистки записей старше " + std::to_wstring(hoursOld) + L" часов";
    HandleEvent(L"CLEAN_START", startMsg.c_str(), false, false);

    int result = g_queue.CleanOldItems(hoursOld, cleanResponses);

    std::wstring completeMsg = L"Очистка завершена. Удалено записей: " + std::to_wstring(result);
    HandleEvent(L"CLEAN_COMPLETE", completeMsg.c_str(), false, false);

    return result;
}

extern "C" __declspec(dllexport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses)
{
    int result = g_queue.GetOldItemsCount(hoursOld, checkResponses);

    std::wstring type = checkResponses ? L"ответов" : L"записей в очереди";
    std::wstring countMsg = L"Найдено " + std::to_wstring(result) + L" " + type + L" старше " + std::to_wstring(hoursOld) + L" часов";

    HandleEvent(L"COUNT_RESULT", countMsg.c_str(), false, false);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Новые экспортируемые функции с флагами управления событиями
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) int __stdcall SendHttpRequestEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent)
{
    HandleEvent(L"SEND_REQUEST_START", L"Начало отправки запроса", useSendEvent, useQueueEvent);
    int result = SendRequestInternalSafe(serverUrl, jsonBody);

    if (result == 0)
        HandleEvent(L"SEND_REQUEST_SUCCESS", L"Запрос успешно отправлен", useSendEvent, useQueueEvent);
    else
        HandleEvent(L"SEND_REQUEST_FAILED", L"Ошибка отправки запроса", useSendEvent, useQueueEvent);

    return result;
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponseEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent)
{
    static thread_local std::wstring responseBuffer;

    HandleEvent(L"SEND_REQUEST_RESPONSE_START", L"Начало отправки запроса с ответом", useSendEvent, useQueueEvent);
    responseBuffer = SendRequestInternalSafeResponse(serverUrl, jsonBody);

    if (responseBuffer.find(L"ERROR:") == 0)
        HandleEvent(L"SEND_REQUEST_RESPONSE_FAILED", responseBuffer.c_str(), useSendEvent, useQueueEvent);
    else
        HandleEvent(L"SEND_REQUEST_RESPONSE_SUCCESS", L"Запрос с ответом успешно обработан", useSendEvent, useQueueEvent);

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall SendHttpRequestQueueEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse, bool useSendEvent, bool useQueueEvent)
{
    if (!serverUrl || !jsonBody) {
        HandleEvent(L"QUEUE_ADD_FAILED", L"Неверные параметры", useSendEvent, useQueueEvent);
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
        HandleEvent(L"QUEUE_ADD_SUCCESS", message.c_str(), useSendEvent, useQueueEvent);
    }
    else {
        HandleEvent(L"QUEUE_ADD_FAILED", L"Ошибка добавления в очередь", useSendEvent, useQueueEvent);
    }

    return result ? 0 : 1;
}

extern "C" __declspec(dllexport) int __stdcall ProcessHttpQueueEx(bool useSendEvent, bool useQueueEvent)
{
    HandleEvent(L"PROCESS_QUEUE_START", L"Запуск обработки очереди в фоновом режиме", useSendEvent, useQueueEvent);

    HANDLE hThread = CreateThread(NULL, 0, ProcessQueueThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        HandleEvent(L"PROCESS_QUEUE_SUCCESS", L"Фоновый поток запущен успешно", useSendEvent, useQueueEvent);
        return 0;
    }
    else {
        HandleEvent(L"PROCESS_QUEUE_FAILED", L"Ошибка запуска фонового потока", useSendEvent, useQueueEvent);
        return 1;
    }
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall GetHttpResponseEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent)
{
    if (!serverUrl || !jsonBody) {
        HandleEvent(L"GET_RESPONSE_FAILED", L"Неверные параметры", useSendEvent, useQueueEvent);
        return L"ERROR: Invalid parameters";
    }

    static thread_local std::wstring responseBuffer;

    size_t urlLen = std::min(wcslen(serverUrl), size_t(2048));
    size_t jsonLen = std::min(wcslen(jsonBody), size_t(8192));

    std::wstring serverUrlW(serverUrl, urlLen);
    std::wstring jsonBodyW(jsonBody, jsonLen);
    std::string jsonBodyUtf8 = WideToUtf8(jsonBodyW.c_str());

    std::string response = g_queue.GetAndRemoveResponse(serverUrlW, jsonBodyUtf8);

    if (!response.empty()) {
        responseBuffer = Utf8ToWide(response.c_str());
        HandleEvent(L"GET_RESPONSE_SUCCESS", L"Ответ успешно получен из базы", useSendEvent, useQueueEvent);
    }
    else {
        responseBuffer = L"ERROR: No response found";
        HandleEvent(L"GET_RESPONSE_NOT_FOUND", L"Ответ не найден в базе", useSendEvent, useQueueEvent);
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall CleanOldHttpItemsEx(int hoursOld, bool cleanResponses, bool useSendEvent, bool useQueueEvent)
{
    std::wstring startMsg = L"Начало очистки записей старше " + std::to_wstring(hoursOld) + L" часов";
    HandleEvent(L"CLEAN_START", startMsg.c_str(), useSendEvent, useQueueEvent);

    int result = g_queue.CleanOldItems(hoursOld, cleanResponses);

    std::wstring completeMsg = L"Очистка завершена. Удалено записей: " + std::to_wstring(result);
    HandleEvent(L"CLEAN_COMPLETE", completeMsg.c_str(), useSendEvent, useQueueEvent);

    return result;
}

extern "C" __declspec(dllexport) int __stdcall GetOldHttpItemsCountEx(int hoursOld, bool checkResponses, bool useSendEvent, bool useQueueEvent)
{
    int result = g_queue.GetOldItemsCount(hoursOld, checkResponses);

    std::wstring type = checkResponses ? L"ответов" : L"записей в очереди";
    std::wstring countMsg = L"Найдено " + std::to_wstring(result) + L" " + type + L" старше " + std::to_wstring(hoursOld) + L" часов";

    HandleEvent(L"COUNT_RESULT", countMsg.c_str(), useSendEvent, useQueueEvent);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Точка входа DLL
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitializeEventsSystem();
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_eventsCs);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}