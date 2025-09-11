#pragma once
#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <windows.h>
#include <string>

/**
 * @typedef EventCallback
 * @brief ��� callback-������� ��� ��������� ������� �� DLL
 * @param event_type ��� ������� � ������� UTF-16
 * @param data ������ ������� � ������� UTF-16
 * @details Callback-������� ����� ���������� DLL ��� ����������� � ��������� ��������
 */
typedef void(__stdcall* EventCallback)(const wchar_t* event_type, const wchar_t* data);

/**
 * @class EventManager
 * @brief �������� ������� ��� callback-�����������
 * @details ������������ �������� �������� ����� ����� DLL � ���������� �����������
 *          ����� callback-�������. ���������� ��� singleton �� ������������ ��������.
 */
class EventManager {
private:
    static EventCallback m_callback;  ///< ��������� �� ������������������ callback-�������

public:
    /**
     * @brief ������������� callback-������� ��� ��������� �������
     * @param callback ��������� �� callback-������� ���� EventCallback
     * @note ���� callback = nullptr, ������� ������������ �� �����
     * @warning �� thread-safe! ��������� �� ��������� ������ ����������
     */
    static void SetCallback(EventCallback callback);

    /**
     * @brief ���������� ������� � ������� � ������� UTF-16
     * @param event_type ��� ������� (UTF-16). ������: L"REQUEST_SUCCESS"
     * @param data ������ ������� (UTF-16). ������: L"������ ������� ���������"
     * @details ���� callback �� ����������, ������� ������������
     * @see SendEvent(const char*, const char*) ��� UTF-8 ������
     */
    static void SendEvent(const wchar_t* event_type, const wchar_t* data);

    /**
     * @brief ���������� ������� � ������� � ������� UTF-8
     * @param event_type ��� ������� (UTF-8). ������: "REQUEST_SUCCESS"
     * @param data ������ ������� (UTF-8). ������: "������ ������� ���������"
     * @details ������������� ������������ UTF-8 � UTF-16 � �������� UTF-16 ������
     * @see SendEvent(const wchar_t*, const wchar_t*) ��� UTF-16 ������
     */
    static void SendEvent(const char* event_type, const char* data);
};

#endif