#ifndef RESULT_H
#define RESULT_H

#include <variant>
#include <string>
#include <QString>

/**
 * @brief Result type for operations that can fail
 * 
 * Similar to Rust's Result<T, E> or C++23's std::expected
 * Provides type-safe error handling without exceptions
 */
template<typename T, typename E = QString>
class Result
{
public:
    // Constructors
    Result(const T& value) : m_data(value) {}
    Result(T&& value) : m_data(std::move(value)) {}
    Result(const E& error) : m_data(error) {}
    Result(E&& error) : m_data(std::move(error)) {}
    
    // Factory methods
    static Result<T, E> ok(const T& value) { return Result(value); }
    static Result<T, E> ok(T&& value) { return Result(std::move(value)); }
    static Result<T, E> error(const E& err) { return Result(err); }
    static Result<T, E> error(E&& err) { return Result(std::move(err)); }
    
    // Check if result contains value or error
    [[nodiscard]] bool isOk() const { return std::holds_alternative<T>(m_data); }
    [[nodiscard]] bool isError() const { return std::holds_alternative<E>(m_data); }
    
    // Get value (throws if error)
    [[nodiscard]] T& value() { return std::get<T>(m_data); }
    [[nodiscard]] const T& value() const { return std::get<T>(m_data); }
    
    // Get error (throws if ok)
    [[nodiscard]] E& error() { return std::get<E>(m_data); }
    [[nodiscard]] const E& error() const { return std::get<E>(m_data); }
    
    // Safe accessors
    [[nodiscard]] T valueOr(const T& defaultValue) const {
        return isOk() ? std::get<T>(m_data) : defaultValue;
    }
    
    // Operator bool for easy checking
    explicit operator bool() const { return isOk(); }

private:
    std::variant<T, E> m_data;
};

// Specialization for void (success/failure only)
template<typename E>
class Result<void, E>
{
public:
    Result() : m_isOk(true) {}
    Result(const E& error) : m_isOk(false), m_error(error) {}
    
    static Result<void, E> ok() { return Result(); }
    static Result<void, E> error(const E& err) { return Result(err); }
    
    [[nodiscard]] bool isOk() const { return m_isOk; }
    [[nodiscard]] bool isError() const { return !m_isOk; }
    [[nodiscard]] const E& error() const { return m_error; }
    
    explicit operator bool() const { return m_isOk; }

private:
    bool m_isOk;
    E m_error;
};

#endif // RESULT_H

