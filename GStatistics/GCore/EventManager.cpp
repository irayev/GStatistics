#include "EventManager.h"
#include "Utilities.h" // для функций конвертации

EventCallback EventManager::m_callback = nullptr;

void EventManager::SetCallback(EventCallback callback)
{
    m_callback = callback;
}

void EventManager::SendEvent(const wchar_t* event_type, const wchar_t* data)
{
    if (m_callback != nullptr)
    {
        m_callback(event_type, data);
    }
}

void EventManager::SendEvent(const char* event_type, const char* data)
{
    if (m_callback != nullptr)
    {
        std::wstring wide_event = Utf8ToWide(event_type);
        std::wstring wide_data = Utf8ToWide(data);
        m_callback(wide_event.c_str(), wide_data.c_str());
    }
}