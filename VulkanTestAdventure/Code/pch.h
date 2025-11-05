#pragma once

// C/C++
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <thread>
#include <streambuf>
#include <span>
#include <unordered_map>
#include <cassert>
#include <tuple>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cmath>
#include <array>
#include <unordered_set>
#include <random>
// NOLINTNEXTLINE(readability-identifier-naming)
#define _USE_MATH_DEFINES
#include <math.h>

// Volk for Vulkan
#include <Volk/volk.h>

// Vulkan Memory Allocator
#include <vma/vk_mem_alloc.h>

// GLFW
// NOLINTNEXTLINE(readability-identifier-naming)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_glfw.h"
//#include "imgui/imgui_impl_vulkan.h"

// GLM
// NOLINTNEXTLINE(readability-identifier-naming)
#define GLM_FORCE_RADIANS
// NOLINTNEXTLINE(readability-identifier-naming)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// NOLINTNEXTLINE(readability-identifier-naming)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

// Utilities
#include "utilities.hpp"
#include "traits.hpp"

// Path Manager
#include "PathManager.hpp"

using namespace vk_test;

#include "Window.hpp"
