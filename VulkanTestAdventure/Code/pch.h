#pragma once

// C/C++
#include <iostream>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <map>

// Vulkan
#include <vulkan/vulkan_raii.hpp>

// GLFW
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>