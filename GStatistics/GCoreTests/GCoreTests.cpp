#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <conio.h>
#include "GStatistics.h"

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

int wmain(int argc, wchar_t* argv[])
{
    // Настраиваем консоль на Unicode
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    if (argc < 2) {
        std::wcout << L"Использование: Tester.exe <serverUrl>\n";
        std::wcout << L"Пример: Tester.exe http://site.ru/base/hs/name/proc/\n";
        return 1;
    }

    const wchar_t* urlW = argv[1];

    std::wstring account = L"1550256932";
    std::wstring broker = L"OnFin Ltd";
    std::wstring message = L"Привет от тестера"; // кириллица теперь нормально отображается

    std::wcout << L"Нажмите любую клавишу для отправки запроса. ESC для выхода.\n";

    while (true)
    {
        int ch = _getch();
        if (ch == 27) break; // ESC

        std::wstring datetime = GetCurrentDateTimeW();

        std::wstring jsonBody = L"{\"DateTime\":\"" + datetime +
            L"\",\"AccountID\":\"" + account +
            L"\",\"BrokerName\":\"" + broker +
            L"\",\"Message\":\"" + message + L"\"}";

        std::wcout << L"\nJSON для отправки:\n" << jsonBody << L"\n";

        int result = SendHttpRequest(urlW, jsonBody.c_str());
        if (result == 0)
            std::wcout << L"Запрос успешно отправлен\n";
        else
            std::wcout << L"Ошибка отправки запроса\n";

        std::wcout << L"\nНажмите любую клавишу для повторной отправки или ESC для выхода.\n";
    }

    return 0;
}