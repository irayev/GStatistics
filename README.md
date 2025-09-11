# GStatistics
# GCore Library

Windows DLL библиотека для асинхронной обработки HTTP-запросов с поддержкой очереди и SQLite-кэшированием

[![Windows](https://img.shields.io/badge/Platform-Windows-0078D6.svg)](https://windows.com)
[![C++](https://img.shields.io/badge/Language-C++-00599C.svg)](https://isocpp.org)
[![SQLite](https://img.shields.io/badge/Database-SQLite-003B57.svg)](https://sqlite.org)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## 🚀 Возможности

- **HTTP Client**: Отправка POST-запросов через WinHTTP
- **Очередь запросов**: Асинхронная обработка с автоматическими повторами
- **SQLite-кэш**: Сохранение запросов и ответов в базе данных
- **Система событий**: Polling-механизм для получения статуса операций
- **Многопоточность**: Фоновая обработка без блокировки основного потока
- **Unicode поддержка**: Полная поддержка UTF-8/UTF-16

## 📦 Установка

### Требования
- Windows 7 или новее
- Visual Studio 2019+ (для сборки)
- SQLite3 (включен в проект)

### Сборка из исходников

```bash
git clone https://github.com/yourusername/GCore.git
cd GCore
mkdir build
cd build
cmake ..
msbuild GCore.sln /p:Configuration=Release
```

## 🏗️ Архитектура

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Внешнее       │    │     GCore       │    │   Удаленный     │
│   приложение    │───▶│     DLL         │───▶│   сервер        │
│   (MQL4/C++/C#) │    │                 │    │   (HTTP/HTTPS)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                            │       ▲
                            ▼       │
                      ┌─────────────────┐
                      │   SQLite база   │
                      │   данных        │
                      └─────────────────┘
```

## 💻 Использование

### Basic Example (C++)

```cpp
#include <iostream>
#include "GCore.h"

int main() {
    // Отправка простого запроса
    int result = SendHttpRequest(
        L"https://api.example.com/data",
        L"{\"key\":\"value\"}"
    );
    
    // Проверка событий
    wchar_t eventType[256];
    wchar_t eventData[1024];
    while (GetNextEvent(eventType, 256, eventData, 1024)) {
        std::wcout << L"Event: " << eventType << L" - " << eventData << std::endl;
    }
    
    return 0;
}
```

### MQL4 Example

```mq4
#import "GCore.dll"
int SendHttpRequest(string serverUrl, string jsonBody);
string SendHttpRequestResponse(string serverUrl, string jsonBody);
int SendHttpRequestQueue(string serverUrl, string jsonBody, bool expectResponse);
int ProcessHttpQueue();
string GetHttpResponse(string serverUrl, string jsonBody);
int CleanOldHttpItems(int hoursOld, bool cleanResponses);
int GetOldHttpItemsCount(int hoursOld, bool checkResponses);
int GetPendingEventCount();
int GetNextEvent(string& eventType, int typeSize, string& eventData, int dataSize);
int ClearEvents();
#import

void OnStart()
{
    // Отправка запроса с получением ответа
    string response = SendHttpRequestResponse(
        "https://api.example.com/rates",
        "{\"symbol\":\"EURUSD\",\"timeframe\":15}"
    );
    
    // Проверка событий
    string eventType, eventData;
    while (GetNextEvent(eventType, 256, eventData, 1024) > 0) {
        Print("Event: ", eventType, " - ", eventData);
    }
    
    Print("Response: ", response);
}
```

## 📚 API Reference

### Основные функции

#### `SendHttpRequest`
```cpp
int SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody);
```
Отправляет HTTP POST запрос без ожидания ответа. Возвращает 0 при успехе, 1 при ошибке.

#### `SendHttpRequestResponse`
```cpp
const wchar_t* SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);
```
Отправляет HTTP POST запрос и возвращает ответ сервера. Возвращает строку ответа или сообщение об ошибке.

#### `SendHttpRequestQueue`
```cpp
int SendHttpRequestQueue(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse);
```
Добавляет запрос в очередь для последующей отправки. Возвращает 0 при успешном добавлении, 1 при ошибке.

#### `ProcessHttpQueue`
```cpp
int ProcessHttpQueue();
```
Запускает фоновую обработку очереди запросов. Возвращает 0 при успешном запуске потока, 1 при ошибке.

#### `GetHttpResponse`
```cpp
const wchar_t* GetHttpResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);
```
Получает ответ из базы данных и удаляет соответствующие записи. Возвращает найденный ответ или сообщение об ошибке.

### Управление базой данных

#### `CleanOldHttpItems`
```cpp
int CleanOldHttpItems(int hoursOld, bool cleanResponses);
```
Очищает старые записи из базы данных. Возвращает количество удаленных записей.

#### `GetOldHttpItemsCount`
```cpp
int GetOldHttpItemsCount(int hoursOld, bool checkResponses);
```
Возвращает количество старых записей в базе данных. Используется для мониторинга и обслуживания БД.

### Система событий (Polling)

#### `GetPendingEventCount`
```cpp
int GetPendingEventCount();
```
Возвращает количество событий, ожидающих обработки в очереди событий.

#### `GetNextEvent`
```cpp
int GetNextEvent(wchar_t* eventType, int typeSize, wchar_t* eventData, int dataSize);
```
Извлекает следующее событие из очереди. Возвращает 1 если событие получено, 0 если очередь пуста.

#### `ClearEvents`
```cpp
int ClearEvents();
```
Очищает всю очередь событий. Возвращает 1 при успешной очистке.

## ⚙️ Конфигурация

### Ограничения
- Максимальная длина URL: 2048 символов
- Максимальная длина тела запроса: 8192 символа
- Поддерживаются только POST-запросы
- Таймаут соединения: 30 секунд

### Настройки базы данных
- Имя файла: `gcore_queue.db`
- Автоматическая очистка: записи старше 24 часов
- Максимальный размер: 100MB

## 🐛 Отладка

Используйте систему событий для отладки:

```mq4
// MQL4 пример отладки
void CheckEvents()
{
    int eventCount = GetPendingEventCount();
    if (eventCount > 0) {
        string eventType, eventData;
        while (GetNextEvent(eventType, 256, eventData, 1024) > 0) {
            Print("DEBUG: ", eventType, " - ", eventData);
        }
    }
}
```

## 📊 Мониторинг

```mq4
// Проверка состояния базы данных
int oldItems = GetOldHttpItemsCount(24, false);
Print("Старых записей в очереди: ", oldItems);

// Очистка базы данных при необходимости
if (oldItems > 1000) {
    int deleted = CleanOldHttpItems(24, true);
    Print("Очищено записей: ", deleted);
}
```

## 🧪 Тестирование

### Пример тестового скрипта (MQL4)

```mq4
// TestGCore.mq4
#import "GCore.dll"
int SendHttpRequest(string serverUrl, string jsonBody);
string SendHttpRequestResponse(string serverUrl, string jsonBody);
int GetPendingEventCount();
int GetNextEvent(string& eventType, int typeSize, string& eventData, int dataSize);
int ClearEvents();
#import

void OnStart()
{
    ClearEvents(); // Очищаем предыдущие события
    
    // Тест 1: Простой запрос
    Print("=== Тест 1: Простой запрос ===");
    int result = SendHttpRequest(
        "https://httpbin.org/post",
        "{\"test\":\"value\",\"number\":123}"
    );
    Print("Результат отправки: ", result);
    
    // Проверяем события
    CheckEvents();
    Sleep(1000);
    
    // Тест 2: Запрос с ответом
    Print("=== Тест 2: Запрос с ответом ===");
    string response = SendHttpRequestResponse(
        "https://httpbin.org/post",
        "{\"action\":\"test\",\"data\":\"hello\"}"
    );
    Print("Ответ сервера: ", response);
    
    CheckEvents();
}

void CheckEvents()
{
    string eventType, eventData;
    int count = GetPendingEventCount();
    Print("Событий в очереди: ", count);
    
    while (GetNextEvent(eventType, 256, eventData, 1024) > 0) {
        Print("Событие: ", eventType, " | Данные: ", eventData);
    }
}
```

## 🤝 Contributing

Мы приветствуем вклад в развитие проекта! 

1. Форкните репозиторий
2. Создайте feature branch (`git checkout -b feature/amazing-feature`)
3. Закоммитьте изменения (`git commit -m 'Add amazing feature'`)
4. Запушьте branch (`git push origin feature/amazing-feature`)
5. Откройте Pull Request

## 📄 Лицензия

Этот проект распространяется под лицензией MIT. Подробнее см. в файле [LICENSE](LICENSE).

## 🆘 Поддержка

Если у вас возникли вопросы или проблемы:

1. Проверьте [Issues](https://github.com/yourusername/GCore/issues)
2. Создайте новое Issue с описанием проблемы
3. Укажите версию ОС и шаги для воспроизведения

## 📈 Статус проекта

![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Downloads](https://img.shields.io/badge/downloads-100+-orange.svg)

---

⭐ Если этот проект был полезен, поставьте звезду на GitHub!
