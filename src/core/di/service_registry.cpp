#include "service_registry.h"

ServiceRegistry& ServiceRegistry::instance()
{
    static ServiceRegistry instance;
    return instance;
}

void ServiceRegistry::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_singletonFactories.clear();
    m_transientFactories.clear();
}

