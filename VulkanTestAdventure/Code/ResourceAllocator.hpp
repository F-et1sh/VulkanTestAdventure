#pragma once
#include "Resources.hpp"

namespace vk_test {
    //--- Resource Allocator ------------------------------------------------------------------------------------------------------------
    //
    // Vulkan Memory Allocator (VMA) is a library that helps to manage memory in Vulkan.
    // This should be used to manage the memory of the resources instead of using the Vulkan API directly.

    class ResourceAllocator {
    public:
        static constexpr VkDeviceSize DEFAULT_LARGE_CHUNK_SIZE = VkDeviceSize(2) * 1024ull * 1024ull * 1024ull;

        ResourceAllocator()                                    = default;
        ResourceAllocator(const ResourceAllocator&)            = delete;
        ResourceAllocator& operator=(const ResourceAllocator&) = delete;
        ResourceAllocator(ResourceAllocator&& other) noexcept;
        ResourceAllocator& operator=(ResourceAllocator&& other) noexcept;
        ~ResourceAllocator();

        operator VmaAllocator() const;

        // Initialization of VMA allocator.
        VkResult init(VmaAllocatorCreateInfo allocatorInfo);

        // De-initialization of VMA allocator.
        void deinit();

        VkDevice         getDevice() const { return m_device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        VkDeviceSize     getMaxMemoryAllocationSize() const { return m_maxMemoryAllocationSize; }

        //////////////////////////////////////////////////////////////////////////

        // Create a VkBuffer
        //
        //        + VMA_MEMORY_USAGE_AUTO
        //        + VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        //        + VMA_MEMORY_USAGE_AUTO_PREFER_HOST
        //      ----
        //        + VMA_ALLOCATION_CREATE_MAPPED_BIT // Automatically maps the buffer upon creation
        //        + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT // If the CPU will sequentially write to the buffer's memory,
        //        + VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        //
        VkResult createBuffer(Buffer&                   buffer,
                              VkDeviceSize              size,
                              VkBufferUsageFlags2KHR    usage,
                              VmaMemoryUsage            memoryUsage   = VMA_MEMORY_USAGE_AUTO,
                              VmaAllocationCreateFlags  flags         = {},
                              VkDeviceSize              minAlignment  = 0,
                              std::span<const uint32_t> queueFamilies = {}) const;

        // This allows more fine control
        VkResult createBuffer(Buffer&                        buffer,
                              const VkBufferCreateInfo&      bufferInfo,
                              const VmaAllocationCreateInfo& allocInfo,
                              VkDeviceSize                   minAlignment = 0) const;

        // Destroy the VkBuffer
        void destroyBuffer(Buffer& buffer) const;

        // A large buffer allows sizes > maxMemoryAllocationSize (often around 4 GB)
        // by using sparse binding and multiple smaller allocations.
        // if no fence is provided, a vkQueueWaitIdle will be performed after the binding
        // operation

        VkResult createLargeBuffer(LargeBuffer&              buffer,
                                   VkDeviceSize              size,
                                   VkBufferUsageFlags2KHR    usage,
                                   VkQueue                   sparseBindingQueue,
                                   VkFence                   sparseBindingFence = VK_NULL_HANDLE,
                                   VkDeviceSize              maxChunkSize       = DEFAULT_LARGE_CHUNK_SIZE,
                                   VmaMemoryUsage            memoryUsage        = VMA_MEMORY_USAGE_AUTO,
                                   VmaAllocationCreateFlags  flags              = {},
                                   VkDeviceSize              minAlignment       = 0,
                                   std::span<const uint32_t> queueFamilies      = {}) const;

        VkResult createLargeBuffer(LargeBuffer&                   buffer,
                                   const VkBufferCreateInfo&      bufferInfo,
                                   const VmaAllocationCreateInfo& allocInfo,
                                   VkQueue                        sparseBindingQueue,
                                   VkFence                        sparseBindingFence = VK_NULL_HANDLE,
                                   VkDeviceSize                   maxChunkSize       = DEFAULT_LARGE_CHUNK_SIZE,
                                   VkDeviceSize                   minAlignment       = 0) const;

        void destroyLargeBuffer(LargeBuffer& buffer) const;

        // Creates VkImage in device memory
        VkResult createImage(Image& image, const VkImageCreateInfo& imageInfo) const;

        // Creates VkImage and VkImageView in device memory
        VkResult createImage(Image& image, const VkImageCreateInfo& imageInfo, const VkImageViewCreateInfo& imageViewInfo) const;

        // Creates VkImage with provided allocation information
        VkResult createImage(Image& image, const VkImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& vmaInfo) const;

        // Creates VkImage and VkImageView with provided allocation information
        VkResult createImage(Image&                         image,
                             const VkImageCreateInfo&       imageInfo,
                             const VkImageViewCreateInfo&   imageViewInfo,
                             const VmaAllocationCreateInfo& vmaInfo) const;

        // Destroys VkImage and VkImageView
        void destroyImage(Image& image) const;

        // AcclerationStructure

        VkResult createAcceleration(AccelerationStructure& accel, const VkAccelerationStructureCreateInfoKHR& accInfo) const;

        VkResult createAcceleration(AccelerationStructure&                      accel,
                                    const VkAccelerationStructureCreateInfoKHR& accInfo,
                                    const VmaAllocationCreateInfo&              vmaInfo,
                                    std::span<const uint32_t>                   queueFamilies = {}) const;

        void destroyAcceleration(AccelerationStructure& accel) const;

        // LargeAccelerationStructure

        // if no fence is provided, a vkQueueWaitIdle will be performed after the binding
        // operation

        VkResult createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                         const VkAccelerationStructureCreateInfoKHR& accInfo,
                                         VkQueue                                     sparseBindingQueue,
                                         VkFence                                     sparseBindingFence = VK_NULL_HANDLE,
                                         VkDeviceSize                                maxChunkSize       = DEFAULT_LARGE_CHUNK_SIZE) const;

        VkResult createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                         const VkAccelerationStructureCreateInfoKHR& accInfo,
                                         const VmaAllocationCreateInfo&              vmaInfo,
                                         VkQueue                                     sparseBindingQueue,
                                         VkFence                                     sparseBindingFence = VK_NULL_HANDLE,
                                         VkDeviceSize                                maxChunkSize       = DEFAULT_LARGE_CHUNK_SIZE,
                                         std::span<const uint32_t>                   queueFamilies      = {}) const;

        void destroyLargeAcceleration(LargeAccelerationStructure& accel) const;

        //////////////////////////////////////////////////////////////////////////

        // When leak are reported, set the ID of the leak here
        void setLeakID(uint32_t id);

        // Returns the device memory of the VMA allocation
        VkDeviceMemory getDeviceMemory(VmaAllocation allocation) const;

        //////////////////////////////////////////////////////////////////////////

        // Calls `vkFlushMappedMemoryRanges` via VMA for the provided buffer's memory.
        // Is required for non-coherent mapped memory after it was written by cpu.
        VkResult flushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // Calls `vkInvalidateMappedMemoryRanges` via VMA for the provided buffer's memory
        // Is required for non-coherent mapped memory prior reading it from cpu.
        VkResult invalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // Calls `vkFlushMappedMemoryRanges` via VMA, if the buffer's memory is non-coherent, otherwise does nothing and always returns VK_SUCCESS.
        // VMA's heuristic for picking memory types, may make it non-trivial to know in advance if a buffer is coherent or not
        VkResult autoFlushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // Calls `vkInvalidateMappedMemoryRanges` via VMA, if the buffer's memory is non-coherent, otherwise does nothing and always returns VK_SUCCESS.
        // VMA's heuristic for picking memory types, may make it non-trivial to know in advance if a buffer is coherent or not
        VkResult autoInvalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    private:
        // Adds "nvvkAllocID: <id>" name to the vma allocation, useful for vma's leak detection
        // (see comments around m_leakID)
        void addLeakDetection(VmaAllocation allocation) const;

    private:
        VmaAllocator     m_allocator{};
        VkDevice         m_device{};
        VkPhysicalDevice m_physicalDevice{};
        VkDeviceSize     m_maxMemoryAllocationSize = 0;

        // Each vma allocation is named using a global monotonic counter
        mutable std::atomic_uint32_t m_allocationCounter = 0;
        // Throws breakpoint/signal when a resource using "nvvkAllocID: <id>" name was
        // created. Only works if `m_allocationCounter` is used deterministically.
        uint32_t m_leakID = ~0U;
    };
} // namespace vk_test
