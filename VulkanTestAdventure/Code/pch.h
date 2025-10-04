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

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

// Vulkan
#include <vulkan/vulkan_raii.hpp>

// GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Utilities
#include "utilities.hpp"
#include "traits.hpp"

// Path Manager
#include "PathManager.h"

using namespace VKTest;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr int MAX_OBJECTS = 3;