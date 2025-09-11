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
 * @brief ��������� ������������ ������� ������� HTTP ��������
 */
struct QueueItem {
    int id;                         ///< ���������� ������������� ������ � ���� ������
    std::wstring server_url;        ///< URL ������� ��� �������� ������� (UTF-16)
    std::string json_body;          ///< ���� JSON ������� (UTF-8)
    bool expect_response;           ///< ���� �������� ������ �� �������
    time_t timestamp;               ///< ��������� ����� �������� ������
};

/**
 * @struct ResponseItem
 * @brief ��������� ������������ ������� ������ �� �������
 */
struct ResponseItem {
    int id;                         ///< ���������� ������������� ������ � ���� ������
    std::wstring server_url;        ///< URL �������, �� �������� ������� ����� (UTF-16)
    std::string request_body;       ///< ���� ������������� ������� (UTF-8)
    std::string response_body;      ///< ���� ������ �� ������� (UTF-8)
    time_t timestamp;               ///< ��������� ����� ��������� ������
};

/**
 * @class SQLiteQueue
 * @brief ����� ��� ���������� �������� HTTP �������� � SQLite ���� ������
 * @details ������������ ����������� ��������� �������� � persistence storage
 */
class SQLiteQueue {
private:
    sqlite3* db;                    ///< ��������� �� ���������� � SQLite �����
    std::string db_path;            ///< ���� � ����� ���� ������

    /**
     * @brief �������������� ���� ������ � ������� ����������� �������
     * @return true ��� �������� �������������, false ��� ������
     */
    bool InitializeDatabase();

    /**
     * @brief ��������� SQL ������ ��� ������������� ����������
     * @param sql SQL ������ ��� ����������
     * @return true ��� �������� ����������, false ��� ������
     */
    bool ExecuteSQL(const std::string& sql);

    bool EnsureFolderExists(const std::string& path);

public:
    
    /**
     * @brief ����������� ������ SQLiteQueue
     * @param database_path ���� � ����� ���� ������ (�� ���������: "http_queue.db")
     */
    SQLiteQueue(const std::string& database_path = "");

    /**
     * @brief ���������� ������ SQLiteQueue
     * @details ��������� ���������� � ����� ������
     */
    ~SQLiteQueue();

    /**
     * @brief ��������� HTTP ������ � ������� ��� ����������� ���������
     * @param server_url URL ������� ��� �������� (UTF-16)
     * @param json_body ���� JSON ������� (UTF-8)
     * @param expect_response ���� �������� ������ �� �������
     * @return true ��� �������� ����������, false ��� ������
     */
    bool AddToQueue(const std::wstring& server_url, const std::string& json_body, bool expect_response);

    /**
     * @brief ���������� ������ ��������, ��������� ���������
     * @param limit ������������ ���������� ������������ �������
     * @return ������ ��������� QueueItem
     */
    std::vector<QueueItem> GetPendingItems(int limit = 100);

    /**
     * @brief ������� ������ �� ������� �� ��������������
     * @param id ������������� ������ ��� ��������
     * @return true ��� �������� ��������, false ��� ������
     */
    bool RemoveFromQueue(int id);

    /**
     * @brief ��������� ����� �� ������� � ���� ������
     * @param server_url URL ������� (UTF-16)
     * @param request_body ���� ������������� ������� (UTF-8)
     * @param response_body ���� ������ �� ������� (UTF-8)
     * @return true ��� �������� ����������, false ��� ������
     */
    bool AddResponse(const std::wstring& server_url, const std::string& request_body, const std::string& response_body);

    /**
     * @brief �������� ����� �� ���� ������ �� URL � ���� �������
     * @param server_url URL ������� ��� ������ (UTF-16)
     * @param request_body ���� ������� ��� ������ (UTF-8)
     * @return ����� ������� ��� ������ ������ ���� �� ������
     */
    std::string GetResponse(const std::wstring& server_url, const std::string& request_body);

    /**
     * @brief �������� ����� �� ���� ������ � ������� ��� matching ������
     * @param server_url URL ������� ��� ������ (UTF-16)
     * @param request_body ���� ������� ��� ������ (UTF-8)
     * @return ����� ������� ��� ������ ������ ���� �� ������
     */
    std::string GetAndRemoveResponse(const std::wstring& server_url, const std::string& request_body);

    /**
     * @brief ������� ����� �� ���� ������ �� ��������������
     * @param id ������������� ������ ��� ��������
     * @return true ��� �������� ��������, false ��� ������
     */
    bool RemoveResponse(int id);

    /**
     * @brief ������� ������ ������ �� ���� ������
     * @param hours_old ������� ������� � ����� ��� ��������
     * @param clean_responses ���� ������� ������� ������� (true) ��� ������ ������� (false)
     * @return ���������� ��������� �������
     */
    int CleanOldItems(int hours_old, bool clean_responses);

    /**
     * @brief ���������� ���������� ������ ������� � ���� ������
     * @param hours_old ������� ������� � ����� ��� ��������
     * @param check_responses ���� �������� ������� ������� (true) ��� ������ ������� (false)
     * @return ���������� ��������� �������
     */
    int GetOldItemsCount(int hours_old, bool check_responses);

    /**
     * @brief ������������ ������� ������� (���������� HTTP ������)
     * @param item ������� ������� ��� ���������
     * @return true ��� �������� ��������, false ��� ������
     */
    bool ProcessQueueItem(const QueueItem& item);
};

#endif