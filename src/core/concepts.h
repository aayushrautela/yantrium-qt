#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>
#include <memory>
#include <QObject>

/**
 * @brief C++20 concepts for type safety
 */

// Concept for QObject-derived types
template<typename T>
concept QObjectDerived = std::derived_from<T, QObject>;

// Concept for services (QObject-derived with Q_OBJECT macro)
template<typename T>
concept ServiceLike = QObjectDerived<T> && requires {
    // Services should be QObject-derived
    typename T::QObject;
};

// Concept for smart pointer types
template<typename T>
concept SmartPointer = requires(T t) {
    t.get();
    *t;
    requires std::same_as<decltype(t.get()), typename T::element_type*>;
};

// Concept for shared ownership
template<typename T>
concept SharedOwnership = SmartPointer<T> && 
    std::same_as<T, std::shared_ptr<typename T::element_type>>;

// Concept for unique ownership
template<typename T>
concept UniqueOwnership = SmartPointer<T> && 
    std::same_as<T, std::unique_ptr<typename T::element_type>>;

#endif // CONCEPTS_H

