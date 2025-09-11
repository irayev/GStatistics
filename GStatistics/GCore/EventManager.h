#pragma once
#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <windows.h>
#include <string>

/**
 * @typedef EventCallback
 * @brief Тип callback-функции для получения событий от DLL
 * @param event_type Тип события в формате UTF-16
 * @param data Данные события в формате UTF-16
 * @details Callback-функция будет вызываться DLL для уведомления о различных событиях
 */
typedef void(__stdcall* EventCallback)(const wchar_t* event_type, const wchar_t* data);

/**
 * @class EventManager
 * @brief Менеджер событий для callback-уведомлений
 * @details Обеспечивает механизм обратной связи между DLL и вызывающим приложением
 *          через callback-функции. Реализован как singleton со статическими методами.
 */
class EventManager {
private:
    static EventCallback m_callback;  ///< Указатель на зарегистрированную callback-функцию

public:
    /**
     * @brief Устанавливает callback-функцию для получения событий
     * @param callback Указатель на callback-функцию типа EventCallback
     * @note Если callback = nullptr, события отправляться не будут
     * @warning Не thread-safe! Вызывайте из основного потока приложения
     */
    static void SetCallback(EventCallback callback);

    /**
     * @brief Отправляет событие с данными в формате UTF-16
     * @param event_type Тип события (UTF-16). Пример: L"REQUEST_SUCCESS"
     * @param data Данные события (UTF-16). Пример: L"Запрос успешно отправлен"
     * @details Если callback не установлен, событие игнорируется
     * @see SendEvent(const char*, const char*) для UTF-8 версии
     */
    static void SendEvent(const wchar_t* event_type, const wchar_t* data);

    /**
     * @brief Отправляет событие с данными в формате UTF-8
     * @param event_type Тип события (UTF-8). Пример: "REQUEST_SUCCESS"
     * @param data Данные события (UTF-8). Пример: "Запрос успешно отправлен"
     * @details Автоматически конвертирует UTF-8 в UTF-16 и вызывает UTF-16 версию
     * @see SendEvent(const wchar_t*, const wchar_t*) для UTF-16 версии
     */
    static void SendEvent(const char* event_type, const char* data);
};

#endif