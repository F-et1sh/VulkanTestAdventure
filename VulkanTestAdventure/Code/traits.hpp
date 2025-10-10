#pragma once

#define VK_TEST_CLASS_DEFAULT(T) \
    T()  = default;              \
    ~T() = default;

#define VK_TEST_CLASS_VIRTUAL(T) \
    T()          = default;      \
    virtual ~T() = default;

#define VK_TEST_CLASS_NONCOPYABLE(T) \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete;

#define VK_TEST_CLASS_MOVABLE_ONLY(T)     \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default; \
    VK_TEST_CLASS_NONCOPYABLE(T)

#define VK_TEST_CLASS_COPYABLE(T)         \
    T(const T&)                = default; \
    T& operator=(const T&)     = default; \
    T(T&&) noexcept            = default; \
    T& operator=(T&&) noexcept = default;

#define VK_TEST_CLASS_STATIC(T)      \
    T()                    = delete; \
    ~T()                   = delete; \
    T(const T&)            = delete; \
    T& operator=(const T&) = delete; \
    T(T&&)                 = delete; \
    T& operator=(T&&)      = delete;
