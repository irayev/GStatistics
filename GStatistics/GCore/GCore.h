// GCore.h - ��������� ����
#pragma once
#ifndef GCORE_H
#define GCORE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	//-----------------------------------------------------------------------------
	// �������� ������� �������� ��������
	//-----------------------------------------------------------------------------

	/**
	 * @brief ���������� HTTP POST ������ ��� �������� ������
	 * @param serverUrl URL ������� ��� �������� ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @return 0 ��� ������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall SendHttpRequest(const wchar_t* serverUrl, const wchar_t* jsonBody);

	/**
	 * @brief ���������� HTTP POST ������ � ���������� ����� �������
	 * @param serverUrl URL ������� ��� �������� ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @return ����� ������� � ���� ������ (UTF-16) ��� ��������� �� ������
	 */
	__declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	/**
	 * @brief ����������� ������: ���������� HTTP POST ������ ��� �������� ������
	 * @param serverUrl URL ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return 0 ��� ������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall SendHttpRequestEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent);

	/**
	 * @brief ����������� ������: ���������� HTTP POST ������ � ���������� ����� �������
	 * @param serverUrl URL ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return ����� ������� � ���� ������ (UTF-16) ��� ��������� �� ������
	 */
	__declspec(dllexport) const wchar_t* __stdcall SendHttpRequestResponseEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent);


	//-----------------------------------------------------------------------------
	// ������� ������ � �������� ��������
	//-----------------------------------------------------------------------------

	/**
	 * @brief ��������� ������ � ������� ��� ����������� ��������
	 * @param serverUrl URL ������� ��� �������� ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @param expectResponse ���� �������� ������ �� ������� (true - ���� �����)
	 * @return 0 ��� �������� ���������� � �������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall SendHttpRequestQueue(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse);

	/**
	 * @brief ��������� ������� ��������� ������� ��������
	 * @return 0 ��� �������� ������� ������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall ProcessHttpQueue();

	/**
	 * @brief �������� ����� �� ���� ������ � ������� ��������������� ������
	 * @param serverUrl URL ������� ��� ������ ������ (UTF-16)
	 * @param jsonBody JSON ���� ������� ��� ������ (UTF-16)
	 * @return ��������� ����� ��� ��������� �� ������ (UTF-16)
	 */
	__declspec(dllexport) const wchar_t* __stdcall GetHttpResponse(const wchar_t* serverUrl, const wchar_t* jsonBody);

	/**
	 * @brief ����������� ������: ��������� ������ � ������� ��� ����������� ��������
	 * @param serverUrl URL ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @param expectResponse ���� �������� ������
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return 0 ��� ������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall SendHttpRequestQueueEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool expectResponse, bool useSendEvent, bool useQueueEvent);

	/**
	 * @brief ����������� ������: ��������� ������� ��������� ������� ��������
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return 0 ��� �������� ������� ������, 1 ��� ������
	 */
	__declspec(dllexport) int __stdcall ProcessHttpQueueEx(bool useSendEvent, bool useQueueEvent);

	/**
	 * @brief ����������� ������: �������� ����� �� ���� ������ � ������� ��������������� ������
	 * @param serverUrl URL ������� (UTF-16)
	 * @param jsonBody JSON ���� ������� (UTF-16)
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return ��������� ����� ��� ��������� �� ������ (UTF-16)
	 */
	__declspec(dllexport) const wchar_t* __stdcall GetHttpResponseEx(const wchar_t* serverUrl, const wchar_t* jsonBody, bool useSendEvent, bool useQueueEvent);


	//-----------------------------------------------------------------------------
	// ������� ���������� ����� ������
	//-----------------------------------------------------------------------------

	/**
	 * @brief ������� ������ ������ �� ���� ������
	 * @param hoursOld ������� ������� � ����� ��� ��������
	 * @param cleanResponses ���� ������� ������� ������� (true) ��� ������ ������� (false)
	 * @return ���������� ��������� �������
	 */
	__declspec(dllexport) int __stdcall CleanOldHttpItems(int hoursOld, bool cleanResponses);

	/**
	 * @brief ���������� ���������� ������ ������� � ���� ������
	 * @param hoursOld ������� ������� � ����� ��� ��������
	 * @param checkResponses ���� �������� ������� ������� (true) ��� ������ ������� (false)
	 * @return ���������� ��������� �������
	 */
	__declspec(dllexport) int __stdcall GetOldHttpItemsCount(int hoursOld, bool checkResponses);

	/**
	 * @brief ����������� ������: ������� ������ ������ �� ���� ������
	 * @param hoursOld ������� ������� � �����
	 * @param cleanResponses true - ������� ������� �������, false - ������ �������
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return ���������� ��������� �������
	 */
	__declspec(dllexport) int __stdcall CleanOldHttpItemsEx(int hoursOld, bool cleanResponses, bool useSendEvent, bool useQueueEvent);

	/**
	 * @brief ����������� ������: ���������� ���������� ������ ������� � ���� ������
	 * @param hoursOld ������� ������� � �����
	 * @param checkResponses true - ��������� ������� �������, false - ������ �������
	 * @param useSendEvent ��������� ��������� ������� ����� EventManager
	 * @param useQueueEvent ��������� ����������� ������� � �������
	 * @return ���������� ��������� �������
	 */
	__declspec(dllexport) int __stdcall GetOldHttpItemsCountEx(int hoursOld, bool checkResponses, bool useSendEvent, bool useQueueEvent);


	//-----------------------------------------------------------------------------
	// ������� callback-�������
	//-----------------------------------------------------------------------------

	/**
	 * @brief ��� callback-������� ��� ��������� ������� �� DLL
	 * @param event_type ��� ������� (UTF-16)
	 * @param data ������ ������� (UTF-16)
	 */
	typedef void(__stdcall* EventCallback)(const wchar_t* event_type, const wchar_t* data);

	/**
	 * @brief ������������� callback-������� ��� ��������� ������� �� DLL
	 * @param callback ��������� �� callback-�������
	 */
	__declspec(dllexport) void __stdcall SetEventCallback(EventCallback callback);


	//-----------------------------------------------------------------------------
	// ������� ��� ������ � ���������
	//-----------------------------------------------------------------------------

	/**
	 * @brief ���������� ���������� �������, ��������� ��������� � ������� �������
	 * @return ���������� ������� � �������
	 */
	__declspec(dllexport) int __stdcall GetPendingEventCount();

	/**
	 * @brief ��������� ��������� ������� �� ������� �������
	 * @param eventType ����� ��� ������ ���� ������� (UTF-16)
	 * @param typeSize ������ ������ ��� ���� �������
	 * @param eventData ����� ��� ������ ������ ������� (UTF-16)
	 * @param dataSize ������ ������ ��� ������ �������
	 * @return 1 ���� ������� ������� ���������, 0 ���� ������� �����
	 */
	__declspec(dllexport) int __stdcall GetNextEvent(wchar_t* eventType, int typeSize, wchar_t* eventData, int dataSize);

	/**
	 * @brief ������� ��� ������� �������
	 * @return 1 ��� �������� �������
	 */
	__declspec(dllexport) int __stdcall ClearEvents();

#ifdef __cplusplus
}
#endif

#endif // GCORE_H