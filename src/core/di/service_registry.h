#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <QObject>

/**
 * @brief Lightweight dependency injection container for managing service lifetimes
 * 
 * Supports both singleton and per-instance registrations with type-safe resolution.
 * Thread-safe for concurrent access.
 */
class ServiceRegistry
{
public:
    using ServiceFactory = std::function<std::shared_ptr<void>()>;
    
    /**
     * @brief Get the singleton instance of the service registry
     */
    static ServiceRegistry& instance();
    
    /**
     * @brief Register a service factory for singleton lifetime
     * @tparam T Service type
     * @param factory Factory function that creates the service
     */
    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>()> factory)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        
        m_singletonFactories[typeIndex] = [factory]() -> std::shared_ptr<void> {
            static std::shared_ptr<T> instance = nullptr;
            if (!instance) {
                instance = factory();
            }
            return std::static_pointer_cast<void>(instance);
        };
    }
    
    /**
     * @brief Register a service factory for per-instance lifetime
     * @tparam T Service type
     * @param factory Factory function that creates the service
     */
    template<typename T>
    void registerTransient(std::function<std::shared_ptr<T>()> factory)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        
        m_transientFactories[typeIndex] = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
    }
    
    /**
     * @brief Register an existing instance as a singleton
     * @tparam T Service type
     * @param instance Shared pointer to the instance
     */
    template<typename T>
    void registerInstance(std::shared_ptr<T> instance)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        
        m_singletonFactories[typeIndex] = [instance]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(instance);
        };
    }
    
    /**
     * @brief Resolve a service instance
     * @tparam T Service type
     * @return Shared pointer to the service instance, or nullptr if not registered
     */
    template<typename T>
    [[nodiscard]] std::shared_ptr<T> resolve()
    {
        ServiceFactory factory;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto typeIndex = std::type_index(typeid(T));
            
            // Try singleton first
            if (auto singletonIt = m_singletonFactories.find(typeIndex); singletonIt != m_singletonFactories.end()) {
                factory = singletonIt->second;
            }
            // Try transient
            else if (auto transientIt = m_transientFactories.find(typeIndex); transientIt != m_transientFactories.end()) {
                factory = transientIt->second;
            } else {
                return nullptr;
            }
        }
        
        // Call factory without holding the lock to avoid deadlock when factory resolves dependencies
        auto voidPtr = factory();
        return std::static_pointer_cast<T>(voidPtr);
    }
    
    /**
     * @brief Check if a service is registered
     * @tparam T Service type
     * @return True if registered, false otherwise
     */
    template<typename T>
    [[nodiscard]] bool isRegistered() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        return m_singletonFactories.find(typeIndex) != m_singletonFactories.end() ||
               m_transientFactories.find(typeIndex) != m_transientFactories.end();
    }
    
    /**
     * @brief Clear all registrations (mainly for testing)
     */
    void clear();

private:
    ServiceRegistry() = default;
    ~ServiceRegistry() = default;
    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    
    std::unordered_map<std::type_index, ServiceFactory> m_singletonFactories;
    std::unordered_map<std::type_index, ServiceFactory> m_transientFactories;
    mutable std::mutex m_mutex;
};

#endif // SERVICE_REGISTRY_H

