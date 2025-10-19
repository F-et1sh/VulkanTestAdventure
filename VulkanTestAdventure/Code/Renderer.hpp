#pragma once

namespace vk_test {
    constexpr inline static int MAX_FRAMES_IN_FLIGHT = 2;

    constexpr inline static std::array VALIDATION_LAYERS{
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr inline static std::array DEVICE_EXTENSIONS{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = false;
#else
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = true;
#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        bool isComplete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription binding_description{};
            binding_description.binding   = 0;
            binding_description.stride    = sizeof(Vertex);
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return binding_description;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

            attribute_descriptions[0].binding  = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
            attribute_descriptions[0].offset   = offsetof(Vertex, pos);

            attribute_descriptions[1].binding  = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attribute_descriptions[1].offset   = offsetof(Vertex, color);

            return attribute_descriptions;
        }
    };

    constexpr inline static std::array VERTICES = {
        Vertex{ { -0.5F, -0.5F }, { 1.0F, 0.0F, 0.0F } },
        Vertex{ { 0.5F, -0.5F }, { 0.0F, 1.0F, 0.0F } },
        Vertex{ { 0.5F, 0.5F }, { 0.0F, 0.0F, 1.0F } },
        Vertex{ { -0.5F, 0.5F }, { 1.0F, 1.0F, 1.0F } }
    };

    constexpr inline static std::array<uint16_t, 6> INDICES = {
        0, 1, 2, 2, 3, 0
    };

    class Renderer {
    public:
        Renderer(Window* window) : p_Window{ window } {}
        ~Renderer() { this->Release(); }

        void Release();
        void Initialize();
        void DrawFrame();

    private:
        void initializeVulkan();
        void initializeImGui();

    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createVertexBuffer();
        void createIndexBuffer();
        void createCommandBuffers();
        void createSyncObjects();

    private:
        void                            releaseSwapchain();
        void                            recreateSwapchain();
        static bool                     checkValidationLayerSupport();
        static void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        static std::vector<const char*> getRequiredExtensions();
        void                            recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
        VkShaderModule                  createShaderModule(const std::vector<char>& code);
        static VkSurfaceFormatKHR       chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        static VkPresentModeKHR         chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
        VkExtent2D                      chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        SwapChainSupportDetails         querySwapChainSupport(VkPhysicalDevice device);
        bool                            isDeviceSuitable(VkPhysicalDevice device);
        static bool                     checkDeviceExtensionSupport(VkPhysicalDevice device);
        QueueFamilyIndices              findQueueFamilies(VkPhysicalDevice device);
        static std::vector<char>        readFile(const std::filesystem::path& filename);
        void                            createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
        void                            copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        uint32_t                        findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

    private:
        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                return func(instance, p_create_info, p_allocator, p_debug_messenger);
            }

            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debug_messenger, p_allocator);
            }
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
            std::cerr << "Validation Layer : " << p_callback_data->pMessage << std::endl;
            return VK_FALSE;
        }

    private:
        Window* p_Window{ nullptr };

        VkInstance               m_Instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };
        VkSurfaceKHR             m_Surface{ VK_NULL_HANDLE };

        VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
        VkDevice         m_Device{ VK_NULL_HANDLE };

        VkQueue m_GraphicsQueue{ VK_NULL_HANDLE };
        VkQueue m_PresentQueue{ VK_NULL_HANDLE };

        VkSwapchainKHR             m_Swapchain{ VK_NULL_HANDLE };
        std::vector<VkImage>       m_SwapchainImages;
        VkFormat                   m_SwapchainImageFormat;
        VkExtent2D                 m_SwapchainExtent{};
        std::vector<VkImageView>   m_SwapchainImageViews;
        std::vector<VkFramebuffer> m_SwapchainFramebuffers;

        VkRenderPass     m_RenderPass{ VK_NULL_HANDLE };
        VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
        VkPipeline       m_GraphicsPipeline{ VK_NULL_HANDLE };

        VkCommandPool                m_CommandPool{ VK_NULL_HANDLE };
        std::vector<VkCommandBuffer> m_CommandBuffers;

        VkBuffer       m_VertexBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_VertexBufferMemory{ VK_NULL_HANDLE };

        VkBuffer       m_IndexBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_IndexBufferMemory{ VK_NULL_HANDLE };

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence>     m_InFlightFences;
        uint32_t                 m_CurrentFrame = 0;
    };
} // namespace vk_test
