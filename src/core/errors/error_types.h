#ifndef ERROR_TYPES_H
#define ERROR_TYPES_H

#include <QString>
#include <variant>

/**
 * @brief Unified error type system
 */

enum class ErrorType {
    None,
    NetworkError,
    AuthenticationError,
    NotFound,
    ValidationError,
    DatabaseError,
    ParseError,
    RateLimitError,
    ServerError,
    TimeoutError,
    UnknownError
};

struct ErrorInfo {
    ErrorType type = ErrorType::None;
    QString message;
    QString context;
    int code = 0;
    
    [[nodiscard]] bool isValid() const { return type != ErrorType::None; }
    
    static ErrorInfo network(const QString& msg, const QString& ctx = QString()) {
        return {ErrorType::NetworkError, msg, ctx, 0};
    }
    
    static ErrorInfo auth(const QString& msg, const QString& ctx = QString()) {
        return {ErrorType::AuthenticationError, msg, ctx, 0};
    }
    
    static ErrorInfo notFound(const QString& msg, const QString& ctx = QString()) {
        return {ErrorType::NotFound, msg, ctx, 0};
    }
    
    static ErrorInfo validation(const QString& msg, const QString& ctx = QString()) {
        return {ErrorType::ValidationError, msg, ctx, 0};
    }
    
    static ErrorInfo database(const QString& msg, const QString& ctx = QString()) {
        return {ErrorType::DatabaseError, msg, ctx, 0};
    }
};

#endif // ERROR_TYPES_H

