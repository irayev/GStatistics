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

// Инициализация критической секции
void InitializeEventsSystem()
{
    InitializeCriticalSection(&g_eventsCs);
}

// Функция для добавления события в очередь
void AddEventToQueue(const wchar_t* event_type, const wchar_t* data)
{
    EnterCriticalSection(&g_eventsCs);
    g_events.push_back(std::make_pair(event_type, data));
    LeaveCriticalSection(&g_eventsCs);
}

// Экспортируемые функции для работы с событиями
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

    // Копируем данные в буферы
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
    AddEventToQueue(L"QUEUE_START", L"Начало обработки очереди");

    std::vector<QueueItem> items = g_queue.GetPendingItems(50);
    int processed = 0;
    int successful = 0;

    std::wstring statusMsg = L"Найдено " + std::to_wstring(items.size()) + L" записей";
    EventManager::SendEvent(L"QUEUE_STATUS", statusMsg.c_str());
    AddEventToQueue(L"QUEUE_STATUS", statusMsg.c_str());

    for (const auto& item : items) {
        processed++;
        if (g_queue.ProcessQueueItem(item)) {
            successful++;
            g_queue.RemoveFromQueue(item.id);
            std::wstring successMsg = L"Успешно отправлен запрос ID: " + std::to_wstring(item.id);
            EventManager::SendEvent(L"REQUEST_SUCCESS", successMsg.c_str());
            AddEventToQueue(L"REQUEST_SUCCESS", successMsg.c_str());
        }
        else {
            std::wstring errorMsg = L"Ошибка отправки запроса ID: " + std::to_wstring(item.id);
            EventManager::SendEvent(L"REQUEST_FAILED", errorMsg.c_str());
            AddEventToQueue(L"REQUEST_FAILED", errorMsg.c_str());
        }
        Sleep(100);
    }

    std::wstring completeMsg = L"Обработка завершена. Успешно: " + std::to_wstring(successful) + L", Всего: " + std::to_wstring(processed);
    EventManager::SendEvent(L"QUEUE_COMPLETE", completeMsg.c_str());
    AddEventToQueue(L"QUEUE_COMPLETE", completeMsg.c_str());

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Экспортируемые функции с callback-событиями
///////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    EventManager::SendEvent(L"SEND_REQUEST_START", L"Начало отправки запроса");
    AddEventToQueue(L"SEND_REQUEST_START", L"Начало отправки запроса");

    int result = SendRequestInternalSafe(serverUrl, jsonBody);

    if (result == 0) {
        EventManager::SendEvent(L"SEND_REQUEST_SUCCESS", L"Запрос успешно отправлен");
        AddEventToQueue(L"SEND_REQUEST_SUCCESS", L"Запрос успешно отправлен");
    }
    else {
        EventManager::SendEvent(L"SEND_REQUEST_FAILED", L"Ошибка отправки запроса");
        AddEventToQueue(L"SEND_REQUEST_FAILED", L"Ошибка отправки запроса");
    }

    return result;
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    static thread_local std::wstring responseBuffer;

    EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_START", L"Начало отправки запроса с ответом");
    AddEventToQueue(L"SEND_REQUEST_RESPONSE_START", L"Начало отправки запроса с ответом");

    responseBuffer = SendRequestInternalSafeResponse(serverUrl, jsonBody);

    if (responseBuffer.find(L"ERROR:") == 0) {
        EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_FAILED", responseBuffer.c_str());
        AddEventToQueue(L"SEND_REQUEST_RESPONSE_FAILED", responseBuffer.c_str());
    }
    else {
        EventManager::SendEvent(L"SEND_REQUEST_RESPONSE_SUCCESS", L"Запрос с ответом успешно обработан");
        AddEventToQueue(L"SEND_REQUEST_RESPONSE_SUCCESS", L"Запрос с ответом успешно обработан");
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall SendHttpRequestQueue(
    const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse)
{
    if (!serverUrl || !jsonBody) {
        EventManager::SendEvent(L"QUEUE_ADD_FAILED", L"Неверные параметры");
        AddEventToQueue(L"QUEUE_ADD_FAILED", L"Неверные параметры");
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
        AddEventToQueue(L"QUEUE_ADD_SUCCESS", message.c_str());
    }
    else {
        EventManager::SendEvent(L"QUEUE_ADD_FAILED", L"Ошибка добавления в очередь");
        AddEventToQueue(L"QUEUE_ADD_FAILED", L"Ошибка добавления в очередь");
    }

    return result ? 0 : 1;
}

extern "C" __declspec(dllexport) int __stdcall ProcessHttpQueue()
{
    EventManager::SendEvent(L"PROCESS_QUEUE_START", L"Запуск обработки очереди в фоновом режиме");
    AddEventToQueue(L"PROCESS_QUEUE_START", L"Запуск обработки очереди в фоновом режиме");

    HANDLE hThread = CreateThread(NULL, 0, ProcessQueueThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        EventManager::SendEvent(L"PROCESS_QUEUE_SUCCESS", L"Фоновый поток запущен успешно");
        AddEventToQueue(L"PROCESS_QUEUE_SUCCESS", L"Фоновый поток запущен успешно");
        return 0;
    }
    else {
        EventManager::SendEvent(L"PROCESS_QUEUE_FAILED", L"Ошибка запуска фонового потока");
        AddEventToQueue(L"PROCESS_QUEUE_FAILED", L"Ошибка запуска фонового потока");
        return 1;
    }
}

extern "C" __declspec(dllexport) const wchar_t* __stdcall GetHttpResponse(
    const wchar_t* serverUrl, const wchar_t* jsonBody)
{
    if (!serverUrl || !jsonBody) {
        EventManager::SendEvent(L"GET_RESPONSE_FAILED", L"Неверные параметры");
        AddEventToQueue(L"GET_RESPONSE_FAILED", L"Неверные параметры");
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
        AddEventToQueue(L"GET_RESPONSE_SUCCESS", L"Ответ успешно получен из базы");
    }
    else {
        responseBuffer = L"ERROR: No response found";
        EventManager::SendEvent(L"GET_RESPONSE_NOT_FOUND", L"Ответ не найден в базе");
        AddEventToQueue(L"GET_RESPONSE_NOT_FOUND", L"Ответ не найден в базе");
    }

    return responseBuffer.c_str();
}

extern "C" __declspec(dllexport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses)
{
    std::wstring startMsg = L"Начало очистки записей старше " + std::to_wstring(hoursOld) + L" часов";
    EventManager::SendEvent(L"CLEAN_START", startMsg.c_str());
    AddEventToQueue(L"CLEAN_START", startMsg.c_str());

    int result = g_queue.CleanOldItems(hoursOld, cleanResponses);

    std::wstring completeMsg = L"Очистка завершена. Удалено записей: " + std::to_wstring(result);
    EventManager::SendEvent(L"CLEAN_COMPLETE", completeMsg.c_str());
    AddEventToQueue(L"CLEAN_COMPLETE", completeMsg.c_str());

    return result;
}

extern "C" __declspec(dllexport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses)
{
    int result = g_queue.GetOldItemsCount(hoursOld, checkResponses);

    std::wstring type = checkResponses ? L"ответов" : L"записей в очереди";
    std::wstring countMsg = L"Найдено " + std::to_wstring(result) + L" " + type + L" старше " + std::to_wstring(hoursOld) + L" часов";

    EventManager::SendEvent(L"COUNT_RESULT", countMsg.c_str());
    AddEventToQueue(L"COUNT_RESULT", countMsg.c_str());

    return result;
}

// Точка входа DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitializeEventsSystem();
        EventManager::SendEvent(L"DLL_LOADED", L"Библиотека GCore загружена");
        AddEventToQueue(L"DLL_LOADED", L"Библиотека GCore загружена");
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_eventsCs);
        EventManager::SendEvent(L"DLL_UNLOADED", L"Библиотека GCore выгружена");
        AddEventToQueue(L"DLL_UNLOADED", L"Библиотека GCore выгружена");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}