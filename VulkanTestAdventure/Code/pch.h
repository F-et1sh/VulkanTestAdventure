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

// GLFW
// NOLINTNEXTLINE(readability-identifier-naming)
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

// Vulkan
#include <vulkan/vulkan_raii.hpp>

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

// Utilities
#include "utilities.hpp"
#include "traits.hpp"

// Path Manager
#include "PathManager.h"

using namespace VKTest;

#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "RenderPassManager.h"
#include "GameObjects.h"
#include "RenderMesh.h"
#include "PipelineManager.h"
