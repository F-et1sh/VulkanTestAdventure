#pragma once
#include "Resources.hpp"

namespace vk_test {
    //--- Resource Allocator ------------------------------------------------------------------------------------------------------------
    //
    // Vulkan Memory Allocator (VMA) is a library that helps to manage memory in Vulkan.
    // This should be used to manage the memory of the resources instead of using the Vulkan API directly.

    class ResourceAllocator {
    public:
        static constexpr VkDeviceSize DEFAULT_LARGE_CHUNK_SIZE = VkDeviceSize(2) * 1024ULL * 1024ULL * 1024ULL;

        ResourceAllocator()                                    = default;
        ResourceAllocator(const ResourceAllocator&)            = delete;
        ResourceAllocator& operator=(const ResourceAllocator&) = delete;
        ResourceAllocator(ResourceAllocator&& other) noexcept;
        ResourceAllocator& operator=(ResourceAllocator&& other) noexcept;
        ~ResourceAllocator();

        operator VmaAllocator() const;

        // Initialization of VMA allocator.
        VkResult init(VmaAllocatorCreateInfo allocator_info);

        // De-initialization of VMA allocator.
        void deinit();

        VkDevice         getDevice() const { return m_Device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
        VkDeviceSize     getMaxMemoryAllocationSize() const { return m_MaxMemoryAllocationSize; }

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
                              VmaMemoryUsage            memory_usage   = VMA_MEMORY_USAGE_AUTO,
                              VmaAllocationCreateFlags  flags          = {},
                              VkDeviceSize              min_alignment  = 0,
                              std::span<const uint32_t> queue_families = {}) const;

        // This allows more fine control
        VkResult createBuffer(Buffer&                        buffer,
                              const VkBufferCreateInfo&      buffer_info,
                              const VmaAllocationCreateInfo& alloc_info,
                              VkDeviceSize                   min_alignment = 0) const;

        // Destroy the VkBuffer
        void destroyBuffer(Buffer& buffer) const;

        // A large buffer allows sizes > maxMemoryAllocationSize (often around 4 GB)
        // by using sparse binding and multiple smaller allocations.
        // if no fence is provided, a vkQueueWaitIdle will be performed after the binding
        // operation

        VkResult createLargeBuffer(LargeBuffer&              buffer,
                                   VkDeviceSize              size,
                                   VkBufferUsageFlags2KHR    usage,
                                   VkQueue                   sparse_binding_queue,
                                   VkFence                   sparse_binding_fence = VK_NULL_HANDLE,
                                   VkDeviceSize              max_chunk_size       = DEFAULT_LARGE_CHUNK_SIZE,
                                   VmaMemoryUsage            memory_usage         = VMA_MEMORY_USAGE_AUTO,
                                   VmaAllocationCreateFlags  flags                = {},
                                   VkDeviceSize              min_alignment        = 0,
                                   std::span<const uint32_t> queue_families       = {}) const;

        VkResult createLargeBuffer(LargeBuffer&                   buffer,
                                   const VkBufferCreateInfo&      buffer_info,
                                   const VmaAllocationCreateInfo& alloc_info,
                                   VkQueue                        sparse_binding_queue,
                                   VkFence                        sparse_binding_fence = VK_NULL_HANDLE,
                                   VkDeviceSize                   max_chunk_size       = DEFAULT_LARGE_CHUNK_SIZE,
                                   VkDeviceSize                   min_alignment        = 0) const;

        void destroyLargeBuffer(LargeBuffer& buffer) const;

        // Creates VkImage in device memory
        VkResult createImage(Image& image, const VkImageCreateInfo& image_info) const;

        // Creates VkImage and VkImageView in device memory
        VkResult createImage(Image& image, const VkImageCreateInfo& image_info, const VkImageViewCreateInfo& image_view_info) const;

        // Creates VkImage with provided allocation information
        VkResult createImage(Image& image, const VkImageCreateInfo& image_info, const VmaAllocationCreateInfo& vma_info) const;

        // Creates VkImage and VkImageView with provided allocation information
        VkResult createImage(Image&                         image,
                             const VkImageCreateInfo&       image_info,
                             const VkImageViewCreateInfo&   image_view_info,
                             const VmaAllocationCreateInfo& vma_info) const;

        // Destroys VkImage and VkImageView
        void destroyImage(Image& image) const;

        // AcclerationStructure

        VkResult createAcceleration(AccelerationStructure& accel, const VkAccelerationStructureCreateInfoKHR& acc_info) const;

        VkResult createAcceleration(AccelerationStructure&                      accel,
                                    const VkAccelerationStructureCreateInfoKHR& acc_info,
                                    const VmaAllocationCreateInfo&              vma_info,
                                    std::span<const uint32_t>                   queue_families = {}) const;

        void destroyAcceleration(AccelerationStructure& accel) const;

        // LargeAccelerationStructure

        // if no fence is provided, a vkQueueWaitIdle will be performed after the binding
        // operation

        VkResult createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                         const VkAccelerationStructureCreateInfoKHR& acc_info,
                                         VkQueue                                     sparse_binding_queue,
                                         VkFence                                     sparse_binding_fence = VK_NULL_HANDLE,
                                         VkDeviceSize                                max_chunk_size       = DEFAULT_LARGE_CHUNK_SIZE) const;

        VkResult createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                         const VkAccelerationStructureCreateInfoKHR& acc_info,
                                         const VmaAllocationCreateInfo&              vma_info,
                                         VkQueue                                     sparse_binding_queue,
                                         VkFence                                     sparse_binding_fence = VK_NULL_HANDLE,
                                         VkDeviceSize                                max_chunk_size       = DEFAULT_LARGE_CHUNK_SIZE,
                                         std::span<const uint32_t>                   queue_families       = {}) const;

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
        VmaAllocator     m_Allocator{};
        VkDevice         m_Device{};
        VkPhysicalDevice m_PhysicalDevice{};
        VkDeviceSize     m_MaxMemoryAllocationSize = 0;

        // Each vma allocation is named using a global monotonic counter
        mutable std::atomic_uint32_t m_AllocationCounter = 0;
        // Throws breakpoint/signal when a resource using "nvvkAllocID: <id>" name was
        // created. Only works if `m_allocationCounter` is used deterministically.
        uint32_t m_LeakID = ~0U;
    };
} // namespace vk_test
