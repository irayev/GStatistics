#pragma once
#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <windows.h>

/**
 * @file Utilities.h
 * @brief Вспомогательные функции для работы с кодировками и HTTP
 */

 /**
  * @brief Конвертирует строку UTF-16 в UTF-8
  * @param wstr Входная строка в UTF-16
  * @return Строка в UTF-8
  */
std::string WideToUtf8(const wchar_t* wstr);

/**
 * @brief Конвертирует строку UTF-8 в UTF-16
 * @param str Входная строка в UTF-8
 * @return Строка в UTF-16
 */
std::wstring Utf8ToWide(const char* str);

/**
 * @brief Отправляет HTTP POST запрос (внутренняя реализация)
 * @param serverUrl URL сервера в UTF-16
 * @param jsonBody Тело запроса в UTF-8
 * @return 0 при успехе, 1 при ошибке
 */
int SendRequestInternal(const std::wstring& serverUrl, const std::string& jsonBody);

/**
 * @brief Отправляет HTTP POST запрос и возвращает ответ (внутренняя реализация)
 * @param serverUrl URL сервера в UTF-16
 * @param jsonBody Тело запроса в UTF-8
 * @return Ответ сервера в UTF-8 или сообщение об ошибке
 */
std::string SendRequestInternalResponse(const std::wstring& serverUrl, const std::string& jsonBody);

#endif