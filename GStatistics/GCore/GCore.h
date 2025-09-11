// GCore.h - отдельный файл
#pragma once
#ifndef GCORE_H
#define GCORE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	//-----------------------------------------------------------------------------
	// Основные функции отправки запросов
	//-----------------------------------------------------------------------------

	/**
	 * @brief Отправляет HTTP POST запрос без ожидания ответа
	 * @param serverUrl URL сервера для отправки запроса (UTF-16)
	 * @param jsonBody JSON тело запроса (UTF-16)
	 * @return 0 при успехе, 1 при ошибке
	 */
	__declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody);

	/**
	 * @brief Отправляет HTTP POST запрос и возвращает ответ сервера
	 * @param serverUrl URL сервера для отправки запроса (UTF-16)
	 * @param jsonBody JSON тело запроса (UTF-16)
	 * @return Ответ сервера в виде строки (UTF-16) или сообщение об ошибке
	 */
	__declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	//-----------------------------------------------------------------------------
	// Функции работы с очередью запросов
	//-----------------------------------------------------------------------------

	/**
	 * @brief Добавляет запрос в очередь для последующей отправки
	 * @param serverUrl URL сервера для отправки запроса (UTF-16)
	 * @param jsonBody JSON тело запроса (UTF-16)
	 * @param expectResponse Флаг ожидания ответа от сервера (true - ждем ответ)
	 * @return 0 при успешном добавлении в очередь, 1 при ошибке
	 */
	__declspec(dllexport) int __stdcall SendHttpRequestQueue(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse);

	/**
	 * @brief Запускает фоновую обработку очереди запросов
	 * @return 0 при успешном запуске потока, 1 при ошибке
	 */
	__declspec(dllexport) int __stdcall ProcessHttpQueue();

	/**
	 * @brief Получает ответ из базы данных и удаляет соответствующие записи
	 * @param serverUrl URL сервера для поиска ответа (UTF-16)
	 * @param jsonBody JSON тело запроса для поиска (UTF-16)
	 * @return Найденный ответ или сообщение об ошибке (UTF-16)
	 */
	__declspec(dllexport) const wchar_t* __stdcall GetHttpResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	//-----------------------------------------------------------------------------
	// Функции управления базой данных
	//-----------------------------------------------------------------------------

	/**
	 * @brief Очищает старые записи из базы данных
	 * @param hoursOld Возраст записей в часах для удаления
	 * @param cleanResponses Флаг очистки таблицы ответов (true) или только очереди (false)
	 * @return Количество удаленных записей
	 */
	__declspec(dllexport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses);

	/**
	 * @brief Возвращает количество старых записей в базе данных
	 * @param hoursOld Возраст записей в часах для проверки
	 * @param checkResponses Флаг проверки таблицы ответов (true) или только очереди (false)
	 * @return Количество найденных записей
	 */
	__declspec(dllexport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses);

	//-----------------------------------------------------------------------------
	// Функции callback-событий
	//-----------------------------------------------------------------------------

	/**
	 * @brief Тип callback-функции для получения событий от DLL
	 * @param event_type Тип события (UTF-16)
	 * @param data Данные события (UTF-16)
	 */
	typedef void(__stdcall* EventCallback)(const wchar_t* event_type, const wchar_t* data);

	/**
	 * @brief Устанавливает callback-функцию для получения событий от DLL
	 * @param callback Указатель на callback-функцию
	 */
	__declspec(dllexport) void __stdcall SetEventCallback(EventCallback callback);


	//-----------------------------------------------------------------------------
	// Функции для работы с событиями
	//-----------------------------------------------------------------------------

	/**
	* @brief Возвращает количество событий, ожидающих обработки в очереди событий
	* @return Количество событий в очереди
	*/
	__declspec(dllexport) int __stdcall GetPendingEventCount();
	
	/**
	 * @brief Извлекает следующее событие из очереди событий
	 * @param eventType Буфер для записи типа события (UTF-16)
	 * @param typeSize Размер буфера для типа события
	 * @param eventData Буфер для записи данных события (UTF-16)
	 * @param dataSize Размер буфера для данных события
	 * @return 1 если событие успешно извлечено, 0 если очередь пуста
	 */
	__declspec(dllexport) int __stdcall GetNextEvent(wchar_t* eventType, int typeSize, wchar_t* eventData, int dataSize);

	/**
	 * @brief Очищает всю очередь событий
	 * @return 1 при успешной очистке
	 */
	__declspec(dllexport) int __stdcall ClearEvents();

#ifdef __cplusplus
}
#endif

#endif // GCORE_H