#include "SQLiteQueue.h"
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <direct.h>   // для _mkdir на Windows


bool SQLiteQueue::EnsureFolderExists(const std::string& path) {
    if (_mkdir(path.c_str()) == 0 || errno == EEXIST) return true;
    return false;
}

SQLiteQueue::SQLiteQueue(const std::string& database_path) : db(nullptr) {
    if (database_path.empty()) {
        std::string folder = "c:\\gcore";
        if (!folder.empty()) EnsureFolderExists(folder);
        db_path = folder + "\\data.db";
    }
    else {
        db_path = database_path;
    }

    InitializeDatabase();
}

SQLiteQueue::~SQLiteQueue() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool SQLiteQueue::InitializeDatabase() {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        return false;
    }

    // Создание таблицы для очереди запросов
    std::string queue_table_sql = R"(
        CREATE TABLE IF NOT EXISTS http_queue (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            server_url TEXT NOT NULL,
            json_body TEXT NOT NULL,
            expect_response INTEGER NOT NULL,
            timestamp INTEGER NOT NULL
        )
    )";

    // Создание таблицы для ответов
    std::string response_table_sql = R"(
        CREATE TABLE IF NOT EXISTS http_responses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            server_url TEXT NOT NULL,
            request_body TEXT NOT NULL,
            response_body TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            UNIQUE(server_url, request_body)
        )
    )";

    return ExecuteSQL(queue_table_sql) && ExecuteSQL(response_table_sql);
}

bool SQLiteQueue::ExecuteSQL(const std::string& sql) {
    if (!db) return false;

    char* err_msg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (result != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return false;
    }

    return true;
}

bool SQLiteQueue::AddToQueue(const std::wstring& server_url, const std::string& json_body, bool expect_response) {
    if (!db) return false;

    std::string sql = R"(
        INSERT INTO http_queue (server_url, json_body, expect_response, timestamp)
        VALUES (?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    std::string url_utf8 = WideToUtf8(server_url.c_str());

    sqlite3_bind_text(stmt, 1, url_utf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, json_body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, expect_response ? 1 : 0);
    sqlite3_bind_int64(stmt, 4, time(nullptr));

    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return result;
}

std::vector<QueueItem> SQLiteQueue::GetPendingItems(int limit) {
    std::vector<QueueItem> items;
    if (!db) return items;

    std::string sql = "SELECT id, server_url, json_body, expect_response, timestamp FROM http_queue ORDER BY timestamp ASC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return items;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QueueItem item;
        item.id = sqlite3_column_int(stmt, 0);

        const unsigned char* url = sqlite3_column_text(stmt, 1);
        if (url) {
            std::string url_str(reinterpret_cast<const char*>(url));
            item.server_url = Utf8ToWide(url_str.c_str());
        }

        const unsigned char* body = sqlite3_column_text(stmt, 2);
        if (body) {
            item.json_body = reinterpret_cast<const char*>(body);
        }

        item.expect_response = sqlite3_column_int(stmt, 3) != 0;
        item.timestamp = sqlite3_column_int64(stmt, 4);

        items.push_back(item);
    }

    sqlite3_finalize(stmt);
    return items;
}

bool SQLiteQueue::RemoveFromQueue(int id) {
    if (!db) return false;

    std::string sql = "DELETE FROM http_queue WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return result;
}

bool SQLiteQueue::AddResponse(const std::wstring& server_url, const std::string& request_body, const std::string& response_body) {
    if (!db) return false;

    std::string sql = R"(
        INSERT OR REPLACE INTO http_responses (server_url, request_body, response_body, timestamp)
        VALUES (?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    std::string url_utf8 = WideToUtf8(server_url.c_str());

    sqlite3_bind_text(stmt, 1, url_utf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, request_body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, response_body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, time(nullptr));

    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return result;
}

std::string SQLiteQueue::GetResponse(const std::wstring& server_url, const std::string& request_body) {
    if (!db) return "";

    std::string sql = "SELECT response_body FROM http_responses WHERE server_url = ? AND request_body = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }

    std::string url_utf8 = WideToUtf8(server_url.c_str());

    sqlite3_bind_text(stmt, 1, url_utf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, request_body.c_str(), -1, SQLITE_TRANSIENT);

    std::string response;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* resp = sqlite3_column_text(stmt, 0);
        if (resp) {
            response = reinterpret_cast<const char*>(resp);
        }
    }

    sqlite3_finalize(stmt);
    return response;
}

std::string SQLiteQueue::GetAndRemoveResponse(const std::wstring& server_url, const std::string& request_body) {
    if (!db) return "";

    // Сначала получаем самый свежий ответ
    std::string sql = R"(
        SELECT response_body 
        FROM http_responses 
        WHERE server_url = ? AND request_body = ? 
        ORDER BY timestamp DESC 
        LIMIT 1
    )";

    sqlite3_stmt* stmt;
    std::string response;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        std::string url_utf8 = WideToUtf8(server_url.c_str());

        sqlite3_bind_text(stmt, 1, url_utf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, request_body.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* resp = sqlite3_column_text(stmt, 0);
            if (resp) {
                response = reinterpret_cast<const char*>(resp);
            }
        }
        sqlite3_finalize(stmt);
    }

    // Если нашли ответ, удаляем ВСЕ записи с такими параметрами
    if (!response.empty()) {
        sql = "DELETE FROM http_responses WHERE server_url = ? AND request_body = ?";

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            std::string url_utf8 = WideToUtf8(server_url.c_str());

            sqlite3_bind_text(stmt, 1, url_utf8.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, request_body.c_str(), -1, SQLITE_TRANSIENT);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    return response;
}

bool SQLiteQueue::RemoveResponse(int id) {
    if (!db) return false;

    std::string sql = "DELETE FROM http_responses WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return result;
}

int SQLiteQueue::CleanOldItems(int hours_old, bool clean_responses) {
    if (!db) return 0;

    time_t cutoff_time = time(nullptr) - (hours_old * 3600);

    std::string sql_queue = "DELETE FROM http_queue WHERE timestamp < ?";
    std::string sql_responses = "DELETE FROM http_responses WHERE timestamp < ?";

    sqlite3_stmt* stmt;
    int deleted_count = 0;

    // Очистка очереди
    if (sqlite3_prepare_v2(db, sql_queue.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, cutoff_time);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            deleted_count += sqlite3_changes(db);
        }
        sqlite3_finalize(stmt);
    }

    // Очистка ответов, если нужно
    if (clean_responses) {
        if (sqlite3_prepare_v2(db, sql_responses.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, cutoff_time);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                deleted_count += sqlite3_changes(db);
            }
            sqlite3_finalize(stmt);
        }
    }

    return deleted_count;
}

int SQLiteQueue::GetOldItemsCount(int hours_old, bool check_responses) {
    if (!db) return 0;

    time_t cutoff_time = time(nullptr) - (hours_old * 3600);

    std::string sql;
    if (check_responses) {
        sql = "SELECT COUNT(*) FROM http_responses WHERE timestamp < ?";
    }
    else {
        sql = "SELECT COUNT(*) FROM http_queue WHERE timestamp < ?";
    }

    sqlite3_stmt* stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, cutoff_time);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return count;
}

bool SQLiteQueue::ProcessQueueItem(const QueueItem& item) {
    std::string body_utf8 = item.json_body;
    std::wstring url_wide = item.server_url;

    if (item.expect_response) {
        std::string response = SendRequestInternalResponse(url_wide, body_utf8);
        if (response.find("ERROR:") != 0) { // Успешный ответ
            AddResponse(url_wide, body_utf8, response);
            return true;
        }
    }
    else {
        int result = SendRequestInternal(url_wide, body_utf8);
        if (result == 0) { // Успешная отправка
            return true;
        }
    }

    return false;
}