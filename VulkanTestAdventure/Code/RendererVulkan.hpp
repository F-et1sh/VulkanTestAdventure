#pragma once
#include "IRenderer.hpp"

namespace vk_test {
    class RendererVulkan : public IRenderer {
    public:
        RendererVulkan(Window* window) : p_Window{ window } {}
        ~RendererVulkan() override = default;

    private:
        Window* p_Window{ nullptr };
    };
} // namespace vk_test
