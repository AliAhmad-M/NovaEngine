#include <Core/Services.h>

using namespace Nova;

Services* Services::s_instanceRef = nullptr;

void Services::Init()
{
    if (s_instanceRef) Debug::LogWarn("Services already initialized");
    s_instanceRef = new Services();
}

void Services::Shutdown()
{
    delete s_instanceRef;
    s_instanceRef = nullptr;
}

Services& Services::Get()
{
    if (!s_instanceRef)
    {
        static Services fallback;
        return fallback;
    }
    return *s_instanceRef;
}