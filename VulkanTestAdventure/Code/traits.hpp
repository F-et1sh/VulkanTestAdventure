#pragma once

#define VKTEST_CLASS_DEFAULT(T) \
    T()  = default;             \
    ~T() = default;

#define VKTEST_CLASS_VIRTUAL(T) \
    T()          = default;     \
    virtual ~T() = default;

#define VKTEST_CLASS_NONCOPYABLE(T)  \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete;

#define VKTEST_CLASS_MOVABLE_ONLY(T)      \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default; \
    VKTEST_CLASS_NONCOPYABLE(T)

#define VKTEST_CLASS_COPYABLE(T)          \
    T(const T&)                = default; \
    T& operator=(const T&)     = default; \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default;

#define VKTEST_CLASS_STATIC(T)       \
    T()                    = delete; \
    ~T()                   = delete; \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete; \
    T(T&&)                 = delete; \
    T& operator=(T&&)      = delete;