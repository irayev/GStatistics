#pragma once
#ifndef SQLITE_QUEUE_H
#define SQLITE_QUEUE_H

#include <string>
#include <vector>
#include "sqlite3.h"
#include <windows.h>
#include "Utilities.h"

/**
 * @struct QueueItem
 * @brief Структура представляет элемент очереди HTTP запросов
 */
struct QueueItem {
    int id;                         ///< Уникальный идентификатор записи в базе данных
    std::wstring server_url;        ///< URL сервера для отправки запроса (UTF-16)
    std::string json_body;          ///< Тело JSON запроса (UTF-8)
    bool expect_response;           ///< Флаг ожидания ответа от сервера
    time_t timestamp;               ///< Временная метка создания записи
};

/**
 * @struct ResponseItem
 * @brief Структура представляет элемент ответа от сервера
 */
struct ResponseItem {
    int id;                         ///< Уникальный идентификатор ответа в базе данных
    std::wstring server_url;        ///< URL сервера, от которого получен ответ (UTF-16)
    std::string request_body;       ///< Тело оригинального запроса (UTF-8)
    std::string response_body;      ///< Тело ответа от сервера (UTF-8)
    time_t timestamp;               ///< Временная метка получения ответа
};

/**
 * @class SQLiteQueue
 * @brief Класс для управления очередью HTTP запросов в SQLite базе данных
 * @details Обеспечивает асинхронную обработку запросов с persistence storage
 */
class SQLiteQueue {
private:
    sqlite3* db;                    ///< Указатель на соединение с SQLite базой
    std::string db_path;            ///< Путь к файлу базы данных

    /**
     * @brief Инициализирует базу данных и создает необходимые таблицы
     * @return true при успешной инициализации, false при ошибке
     */
    bool InitializeDatabase();

    /**
     * @brief Выполняет SQL запрос без возвращаемого результата
     * @param sql SQL запрос для выполнения
     * @return true при успешном выполнении, false при ошибке
     */
    bool ExecuteSQL(const std::string& sql);

    bool EnsureFolderExists(const std::string& path);

public:
    
    /**
     * @brief Конструктор класса SQLiteQueue
     * @param database_path Путь к файлу базы данных (по умолчанию: "http_queue.db")
     */
    SQLiteQueue(const std::string& database_path = "");

    /**
     * @brief Деструктор класса SQLiteQueue
     * @details Закрывает соединение с базой данных
     */
    ~SQLiteQueue();

    /**
     * @brief Добавляет HTTP запрос в очередь для последующей обработки
     * @param server_url URL сервера для отправки (UTF-16)
     * @param json_body Тело JSON запроса (UTF-8)
     * @param expect_response Флаг ожидания ответа от сервера
     * @return true при успешном добавлении, false при ошибке
     */
    bool AddToQueue(const std::wstring& server_url, const std::string& json_body, bool expect_response);

    /**
     * @brief Возвращает список запросов, ожидающих обработки
     * @param limit Максимальное количество возвращаемых записей
     * @return Вектор элементов QueueItem
     */
    std::vector<QueueItem> GetPendingItems(int limit = 100);

    /**
     * @brief Удаляет запрос из очереди по идентификатору
     * @param id Идентификатор записи для удаления
     * @return true при успешном удалении, false при ошибке
     */
    bool RemoveFromQueue(int id);

    /**
     * @brief Добавляет ответ от сервера в базу данных
     * @param server_url URL сервера (UTF-16)
     * @param request_body Тело оригинального запроса (UTF-8)
     * @param response_body Тело ответа от сервера (UTF-8)
     * @return true при успешном добавлении, false при ошибке
     */
    bool AddResponse(const std::wstring& server_url, const std::string& request_body, const std::string& response_body);

    /**
     * @brief Получает ответ из базы данных по URL и телу запроса
     * @param server_url URL сервера для поиска (UTF-16)
     * @param request_body Тело запроса для поиска (UTF-8)
     * @return Ответ сервера или пустая строка если не найден
     */
    std::string GetResponse(const std::wstring& server_url, const std::string& request_body);

    /**
     * @brief Получает ответ из базы данных и удаляет все matching записи
     * @param server_url URL сервера для поиска (UTF-16)
     * @param request_body Тело запроса для поиска (UTF-8)
     * @return Ответ сервера или пустая строка если не найден
     */
    std::string GetAndRemoveResponse(const std::wstring& server_url, const std::string& request_body);

    /**
     * @brief Удаляет ответ из базы данных по идентификатору
     * @param id Идентификатор ответа для удаления
     * @return true при успешном удалении, false при ошибке
     */
    bool RemoveResponse(int id);

    /**
     * @brief Очищает старые записи из базы данных
     * @param hours_old Возраст записей в часах для удаления
     * @param clean_responses Флаг очистки таблицы ответов (true) или только очереди (false)
     * @return Количество удаленных записей
     */
    int CleanOldItems(int hours_old, bool clean_responses);

    /**
     * @brief Возвращает количество старых записей в базе данных
     * @param hours_old Возраст записей в часах для проверки
     * @param check_responses Флаг проверки таблицы ответов (true) или только очереди (false)
     * @return Количество найденных записей
     */
    int GetOldItemsCount(int hours_old, bool check_responses);

    /**
     * @brief Обрабатывает элемент очереди (отправляет HTTP запрос)
     * @param item Элемент очереди для обработки
     * @return true при успешной отправке, false при ошибке
     */
    bool ProcessQueueItem(const QueueItem& item);
};

#endif