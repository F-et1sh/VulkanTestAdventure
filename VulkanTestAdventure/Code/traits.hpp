#pragma once

#define CLASS_DEFAULT(T) \
    T()  = default;      \
    ~T() = default;

#define CLASS_VIRTUAL(T)    \
    T()          = default; \
    virtual ~T() = default;

#define CLASS_NONCOPYABLE(T)         \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete;

#define CLASS_MOVABLE_ONLY(T)             \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default; \
    CLASS_NONCOPYABLE(T)

#define CLASS_COPYABLE(T)                 \
    T(const T&)                = default; \
    T& operator=(const T&)     = default; \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default;

#define CLASS_STATIC(T)              \
    T()                    = delete; \
    ~T()                   = delete; \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete; \
    T(T&&)                 = delete; \
    T& operator=(T&&)      = delete;\
