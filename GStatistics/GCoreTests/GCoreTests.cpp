#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <vector>
#include <algorithm>
#include <map>
#include "GCore.h"

// Forward declarations
void TestSendHttpRequest(const wchar_t* urlW);
void TestSendHttpRequestResponse(const wchar_t* urlW);
void TestSendHttpRequestQueue(const wchar_t* urlW, bool expectResponse);
void TestProcessHttpQueue();
void TestGetHttpResponse(const wchar_t* urlW);
void TestCleanOldHttpItems();
void TestGetOldHttpItemsCount();
void TestAllMethods(const wchar_t* urlW);
void TestResponseWorkflow(const wchar_t* urlW);
void TestDetailedResponseAnalysis(const wchar_t* urlW);
void TestCallbackEvents();
void PrintMenu();
int ReadMenuOption();

// Глобальные переменные для статистики событий
std::map<std::wstring, int> eventStatistics;
bool callbackRegistered = false;

// Callback-функция для получения событий
void __stdcall EventCallbackHandler(const wchar_t* event_type, const wchar_t* data)
{
    std::wstring eventStr(event_type);
    std::wstring dataStr(data);

    // Сохраняем статистику
    eventStatistics[eventStr]++;

    // Выводим событие
    std::wcout << L"📢 CALLBACK: [" << eventStr << L"] " << dataStr << L"\n";
}

// Получаем текущее время
std::wstring GetCurrentDateTimeW()
{
    time_t now = time(nullptr);
    struct tm tstruct;
    wchar_t buf[20];
    localtime_s(&tstruct, &now);
    wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tstruct);
    return buf;
}

// Проверяет, является ли ответ ошибкой
bool IsErrorResponse(const std::wstring& response)
{
    return response.find(L"ERROR:") == 0 || response.find(L"error") != std::wstring::npos;
}

// Проверяет, является ли ответ успешным
bool IsSuccessResponse(const std::wstring& response)
{
    return !IsErrorResponse(response) && !response.empty() && response != L"ERROR: No response found";
}

// Детальный анализ ответа
void AnalyzeResponse(const std::wstring& response, const std::wstring& functionName)
{
    std::wcout << L"\n--- Анализ ответа для " << functionName << L" ---\n";
    std::wcout << L"Ответ: " << response << L"\n";

    if (response.empty()) {
        std::wcout << L"❌ Пустой ответ\n";
        return;
    }

    if (IsErrorResponse(response)) {
        std::wcout << L"❌ Обнаружена ошибка\n";

        if (response.find(L"HTTP") != std::wstring::npos) {
            std::wcout << L"   HTTP ошибка сервера\n";
        }
        else if (response.find(L"Failed") != std::wstring::npos) {
            std::wcout << L"   Ошибка выполнения операции\n";
        }
        else if (response.find(L"No response") != std::wstring::npos) {
            std::wcout << L"   Ответ не найден в базе\n";
        }
    }
    else {
        std::wcout << L"✅ Успешный ответ\n";

        if (response.find(L'{') != std::wstring::npos && response.find(L'}') != std::wstring::npos) {
            std::wcout << L"   Похоже на JSON ответ\n";
        }

        std::wcout << L"   Длина ответа: " << response.length() << L" символов\n";

        if (response.find(L"success") != std::wstring::npos || response.find(L"result") != std::wstring::npos) {
            std::wcout << L"   Содержит поля результата\n";
        }
    }
}

// Регистрирует callback если еще не зарегистрирован
void EnsureCallbackRegistered()
{
    if (!callbackRegistered) {
        SetEventCallback(EventCallbackHandler);
        callbackRegistered = true;
        std::wcout << L"✅ Callback зарегистрирован\n";
    }
}

void TestSendHttpRequest(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wstring datetime = GetCurrentDateTimeW();
    std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
        L"\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"Тест обычной отправки\"}";

    std::wcout << L"\n--- Тест SendHttpRequest ---\n";
    std::wcout << L"URL: " << urlW << L"\n";
    std::wcout << L"JSON: " << jsonBody << L"\n";

    int result = SendHttpRequest(urlW, jsonBody.c_str());
    std::wcout << L"Результат: " << (result == 0 ? L"✅ Успех" : L"❌ Ошибка") << L" (" << result << L")\n";
}

void TestSendHttpRequestResponse(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wstring datetime = GetCurrentDateTimeW();
    std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
        L"\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"Тест отправки с ответом\"}";

    std::wcout << L"\n--- Тест SendHttpRequestResponse ---\n";
    std::wcout << L"URL: " << urlW << L"\n";
    std::wcout << L"JSON: " << jsonBody << L"\n";

    const wchar_t* response = SendHttpRequestResponse(urlW, jsonBody.c_str());
    std::wstring responseStr(response);
    std::wcout << L"Ответ сервера: " << responseStr << L"\n";

    AnalyzeResponse(responseStr, L"SendHttpRequestResponse");
}

void TestSendHttpRequestQueue(const wchar_t* urlW, bool expectResponse)
{
    EnsureCallbackRegistered();

    std::wstring datetime = GetCurrentDateTimeW();
    std::wstring message = expectResponse ? L"Тест очереди с ответом" : L"Тест очереди без ответа";
    std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
        L"\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"" + message + L"\"}";

    std::wcout << L"\n--- Тест SendHttpRequestQueue ---\n";
    std::wcout << L"URL: " << urlW << L"\n";
    std::wcout << L"Ожидать ответ: " << (expectResponse ? L"✅ Да" : L"❌ Нет") << L"\n";
    std::wcout << L"JSON: " << jsonBody << L"\n";

    int result = SendHttpRequestQueue(urlW, jsonBody.c_str(), expectResponse);
    std::wcout << L"Результат добавления в очередь: " << (result == 0 ? L"✅ Успех" : L"❌ Ошибка") << L" (" << result << L")\n";
}

void TestProcessHttpQueue()
{
    EnsureCallbackRegistered();

    std::wcout << L"\n--- Тест ProcessHttpQueue ---\n";
    int result = ProcessHttpQueue();
    std::wcout << L"Результат обработки очереди: " << (result == 0 ? L"✅ Успех" : L"❌ Ошибка") << L" (" << result << L")\n";
    std::wcout << L"Обработка запущена в фоновом режиме\n";
}

void TestGetHttpResponse(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wstring jsonBody = L"{\"DateTime\":\"2024-01-01 12:00:00\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"Тест получения ответа\"}";

    std::wcout << L"\n--- Тест GetHttpResponse ---\n";
    std::wcout << L"URL: " << urlW << L"\n";
    std::wcout << L"JSON для поиска: " << jsonBody << L"\n";

    const wchar_t* response = GetHttpResponse(urlW, jsonBody.c_str());
    std::wstring responseStr(response);
    std::wcout << L"Найденный ответ: " << responseStr << L"\n";

    AnalyzeResponse(responseStr, L"GetHttpResponse");
}

void TestCleanOldHttpItems()
{
    EnsureCallbackRegistered();

    std::wcout << L"\n--- Тест CleanOldHttpItems ---\n";

    std::vector<std::pair<int, bool>> testCases = {
        {1, false}, {24, false}, {1, true}, {24, true}
    };

    for (size_t i = 0; i < testCases.size(); ++i) {
        int hoursOld = testCases[i].first;
        bool cleanResponses = testCases[i].second;

        int result = CleanOldHttpItems(hoursOld, cleanResponses);
        std::wcout << L"Удалено записей старше " << hoursOld << L" часов ("
            << (cleanResponses ? L"с ответами" : L"без ответов") << L"): " << result << L"\n";
    }
}

void TestGetOldHttpItemsCount()
{
    EnsureCallbackRegistered();

    std::wcout << L"\n--- Тест GetOldHttpItemsCount ---\n";

    std::vector<int> hoursToTest = { 1, 6, 12, 24, 48, 168 };

    std::wcout << L"Записей в очереди:\n";
    for (size_t i = 0; i < hoursToTest.size(); ++i) {
        int count = GetOldHttpItemsCount(hoursToTest[i], false);
        std::wcout << L"  старше " << hoursToTest[i] << L" часов: " << count << L"\n";
    }

    std::wcout << L"Ответов в базе:\n";
    for (size_t i = 0; i < hoursToTest.size(); ++i) {
        int count = GetOldHttpItemsCount(hoursToTest[i], true);
        std::wcout << L"  старше " << hoursToTest[i] << L" часов: " << count << L"\n";
    }
}

void TestResponseWorkflow(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wcout << L"\n=== Тест workflow с ответами ===\n";

    std::wstring datetime = GetCurrentDateTimeW();
    std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
        L"\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"Тест workflow ответа\"}";

    std::wcout << L"1. Добавляем в очередь: " << jsonBody << L"\n";
    int addResult = SendHttpRequestQueue(urlW, jsonBody.c_str(), true);
    std::wcout << L"   Результат: " << (addResult == 0 ? L"✅ Успех" : L"❌ Ошибка") << L"\n";

    std::wcout << L"2. Обрабатываем очередь...\n";
    int processResult = ProcessHttpQueue();
    std::wcout << L"   Результат: " << (processResult == 0 ? L"✅ Успех" : L"❌ Ошибка") << L"\n";

    std::wcout << L"3. Ожидание обработки (3 секунды)...\n";
    Sleep(3000);

    std::wcout << L"4. Пытаемся получить ответ...\n";
    const wchar_t* response = GetHttpResponse(urlW, jsonBody.c_str());
    std::wstring responseStr(response);
    std::wcout << L"   Ответ: " << responseStr << L"\n";

    AnalyzeResponse(responseStr, L"ResponseWorkflow");
}

void TestDetailedResponseAnalysis(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wcout << L"\n=== Детальный анализ ответов ===\n";

    std::wstring testMessages[] = {
        L"Простой тест", L"Тест с кириллицей: привет мир",
        L"Тест с спецсимволами: !@#$%^&*()", L"Тест с числами: 1234567890",
        L"Тест с JSON данными"
    };

    for (size_t i = 0; i < sizeof(testMessages) / sizeof(testMessages[0]); ++i) {
        std::wcout << L"\n--- Тест " << (i + 1) << L" из " << (sizeof(testMessages) / sizeof(testMessages[0])) << L" ---\n";

        std::wstring datetime = GetCurrentDateTimeW();
        std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
            L"\",\"AccountID\":\"1550256932\",\"BrokerName\":\"OnFin Ltd\",\"Message\":\"" + testMessages[i] + L"\"}";

        std::wcout << L"Сообщение: " << testMessages[i] << L"\n";

        const wchar_t* response = SendHttpRequestResponse(urlW, jsonBody.c_str());
        std::wstring responseStr(response);

        AnalyzeResponse(responseStr, L"DetailedTest_" + std::to_wstring(i + 1));
        Sleep(1000);
    }
}

void TestCallbackEvents()
{
    EnsureCallbackRegistered();

    std::wcout << L"\n=== Тест callback-событий ===\n";
    std::wcout << L"Отправляем тестовые события...\n";

    // Тестируем разные типы событий
    SendHttpRequest(L"http://test.com", L"{\"test\":\"callback\"}");
    Sleep(500);

    SendHttpRequestQueue(L"http://test.com", L"{\"test\":\"queue\"}", true);
    Sleep(500);

    ProcessHttpQueue();
    Sleep(500);

    CleanOldHttpItems(24, true);
    Sleep(500);

    std::wcout << L"\n📊 Статистика полученных событий:\n";
    for (std::map<std::wstring, int>::iterator it = eventStatistics.begin(); it != eventStatistics.end(); ++it) {
        std::wcout << L"  " << it->first << L": " << it->second << L" событий\n";
    }

    if (eventStatistics.empty()) {
        std::wcout << L"❌ События не получены. Проверьте регистрацию callback\n";
    }
    else {
        std::wcout << L"✅ Callback работает корректно\n";
    }
}

void TestAllMethods(const wchar_t* urlW)
{
    EnsureCallbackRegistered();

    std::wcout << L"\n=== Тестирование всех методов ===\n";

    TestSendHttpRequest(urlW);
    Sleep(1000);
    TestSendHttpRequestResponse(urlW);
    Sleep(1000);
    TestSendHttpRequestQueue(urlW, false);
    Sleep(1000);
    TestSendHttpRequestQueue(urlW, true);
    Sleep(1000);
    TestProcessHttpQueue();
    Sleep(2000);
    TestGetHttpResponse(urlW);
    Sleep(1000);
    TestCleanOldHttpItems();
    Sleep(1000);
    TestGetOldHttpItemsCount();

    std::wcout << L"\n=== Все тесты завершены ===\n";
}

void PrintMenu()
{
    std::wcout << L"\n=== Тестер GCore DLL ===\n";
    std::wcout << L"1. SendHttpRequest - обычная отправка\n";
    std::wcout << L"2. SendHttpRequestResponse - отправка с ответом\n";
    std::wcout << L"3. SendHttpRequestQueue - добавление в очередь\n";
    std::wcout << L"4. ProcessHttpQueue - обработка очереди\n";
    std::wcout << L"5. GetHttpResponse - получение ответа из базы\n";
    std::wcout << L"6. CleanOldHttpItems - очистка старых записей\n";
    std::wcout << L"7. GetOldHttpItemsCount - количество старых записей\n";
    std::wcout << L"8. Тест всех методов\n";
    std::wcout << L"9. Тест workflow с ответами\n";
    std::wcout << L"10. Детальный анализ ответов\n";
    std::wcout << L"11. Тест callback-событий\n";
    std::wcout << L"12. Показать статистику событий\n";
    std::wcout << L"0. Выход\n";
    std::wcout << L"Выберите опцию (0-12): ";
}

int ReadMenuOption()
{
    std::wstring input;
    wchar_t ch;

    while (true) {
        ch = _getch();

        if (ch == 27) return -1; // ESC
        else if (ch == 13) { // Enter
            if (!input.empty()) {
                try { return std::stoi(input); }
                catch (...) {
                    std::wcout << L"\n❌ Неверный ввод. Попробуйте снова: ";
                    input.clear();
                    continue;
                }
            }
        }
        else if (ch == 8) { // Backspace
            if (!input.empty()) {
                input.pop_back();
                std::wcout << L"\b \b";
            }
        }
        else if (ch >= L'0' && ch <= L'9') {
            input += ch;
            std::wcout << ch;
        }
    }
}

int wmain(int argc, wchar_t* argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    if (argc < 2) {
        std::wcout << L"Использование: Tester.exe <serverUrl>\n";
        std::wcout << L"Пример: Tester.exe http://site.ru/base/hs/name/proc/\n";
        return 1;
    }

    const wchar_t* urlW = argv[1];

    // Регистрируем callback сразу
    SetEventCallback(EventCallbackHandler);
    callbackRegistered = true;
    std::wcout << L"✅ Callback зарегистрирован\n";

    while (true) {
        PrintMenu();
        int option = ReadMenuOption();

        if (option == -1 || option == 0) break;

        switch (option) {
        case 1: TestSendHttpRequest(urlW); break;
        case 2: TestSendHttpRequestResponse(urlW); break;
        case 3: TestSendHttpRequestQueue(urlW, false); break;
        case 4: TestSendHttpRequestQueue(urlW, true); break;
        case 5: TestProcessHttpQueue(); break;
        case 6: TestGetHttpResponse(urlW); break;
        case 7: TestCleanOldHttpItems(); break;
        case 8: TestGetOldHttpItemsCount(); break;
        case 9: TestAllMethods(urlW); break;
        case 10: TestResponseWorkflow(urlW); break;
        case 11: TestDetailedResponseAnalysis(urlW); break;
        case 12:
            std::wcout << L"\n📊 Статистика событий:\n";
            for (std::map<std::wstring, int>::iterator it = eventStatistics.begin(); it != eventStatistics.end(); ++it) {
                std::wcout << L"  " << it->first << L": " << it->second << L" событий\n";
            }
            if (eventStatistics.empty()) {
                std::wcout << L"  Нет событий\n";
            }
            break;
        default: std::wcout << L"\n❌ Неизвестная опция\n"; break;
        }

        std::wcout << L"\nНажмите Enter для продолжения...";
        while (_getch() != 13);
    }

    return 0;
}