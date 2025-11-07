#include "pch.h"
#include "resource_allocator.hpp"

// Vulkan Memory Allocator
// NOLINTNEXTLINE(readability-identifier-naming)
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

vk_test::ResourceAllocator::ResourceAllocator(ResourceAllocator&& other) noexcept {
    std::swap(m_Allocator, other.m_Allocator);
    std::swap(m_Device, other.m_Device);
    std::swap(m_PhysicalDevice, other.m_PhysicalDevice);
    std::swap(m_LeakID, other.m_LeakID);
    std::swap(m_MaxMemoryAllocationSize, other.m_MaxMemoryAllocationSize);
}

vk_test::ResourceAllocator& vk_test::ResourceAllocator::operator=(ResourceAllocator&& other) noexcept {
    if (this != &other) {
        assert(m_Allocator == nullptr && "Missing deinit()");

        std::swap(m_Allocator, other.m_Allocator);
        std::swap(m_Device, other.m_Device);
        std::swap(m_PhysicalDevice, other.m_PhysicalDevice);
        std::swap(m_LeakID, other.m_LeakID);
        std::swap(m_MaxMemoryAllocationSize, other.m_MaxMemoryAllocationSize);
    }

    return *this;
}

vk_test::ResourceAllocator::~ResourceAllocator() {
    assert(m_Allocator == nullptr && "Missing deinit()");
}

vk_test::ResourceAllocator::operator VmaAllocator() const {
    return m_Allocator;
}

VkResult vk_test::ResourceAllocator::init(VmaAllocatorCreateInfo allocator_info) {
    assert(m_Allocator == nullptr);

    // #TODO : VK_EXT_memory_priority ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT

    allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // allow querying for the GPU address of a buffer
    allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT; // allow using VkBufferUsageFlags2CreateInfoKHR

    VkPhysicalDeviceVulkan11Properties props11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
    };

    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &props11,
    };
    vkGetPhysicalDeviceProperties2(allocator_info.physicalDevice, &props);
    m_MaxMemoryAllocationSize = props11.maxMemoryAllocationSize;

    VK_TEST_SAY("Max size : " << m_MaxMemoryAllocationSize);

    m_Device         = allocator_info.device;
    m_PhysicalDevice = allocator_info.physicalDevice;

    // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };
    allocator_info.pVulkanFunctions = &functions;
    return vmaCreateAllocator(&allocator_info, &m_Allocator);
}

void vk_test::ResourceAllocator::deinit() {
    if (m_Allocator == nullptr) {
        return;
    }

    vmaDestroyAllocator(m_Allocator);
    m_Allocator      = nullptr;
    m_Device         = nullptr;
    m_PhysicalDevice = nullptr;
    m_LeakID         = ~0;
}

void vk_test::ResourceAllocator::addLeakDetection(VmaAllocation allocation) const {
    /*if(m_LeakID == m_allocationCounter)
  {
#ifdef _WIN32
    if(IsDebuggerPresent())
    {
      DebugBreak();
    }
#elif defined(__unix__)
    raise(SIGTRAP);
#endif
  }
  std::string nvvkAllocID = fmt::format("nvvkAllocID: {}", m_allocationCounter++);
  vmaSetAllocationName(m_Allocator, allocation, nvvkAllocID.c_str());*/
}

VkResult vk_test::ResourceAllocator::createBuffer(vk_test::Buffer&          buffer,
                                                  VkDeviceSize              size,
                                                  VkBufferUsageFlags2KHR    usage,
                                                  VmaMemoryUsage            memory_usage,
                                                  VmaAllocationCreateFlags  flags,
                                                  VkDeviceSize              min_alignment,
                                                  std::span<const uint32_t> queue_families) const {
    const VkBufferUsageFlags2CreateInfo buffer_usage_flags2_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = usage | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
    };

    const VkBufferCreateInfo buffer_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &buffer_usage_flags2_create_info,
        .size                  = size,
        .usage                 = 0,
        .sharingMode           = queue_families.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size()),
        .pQueueFamilyIndices   = queue_families.data(),
    };

    VmaAllocationCreateInfo alloc_info = { .flags = flags, .usage = memory_usage };

    return createBuffer(buffer, buffer_info, alloc_info, min_alignment);
}

VkResult vk_test::ResourceAllocator::createBuffer(vk_test::Buffer&               result_buffer,
                                                  const VkBufferCreateInfo&      buffer_info,
                                                  const VmaAllocationCreateInfo& alloc_info,
                                                  VkDeviceSize                   min_alignment) const {
    result_buffer = {};

    // Create the buffer
    VmaAllocationInfo alloc_info_out{};

    VkResult result = vmaCreateBufferWithAlignment(m_Allocator, &buffer_info, &alloc_info, min_alignment, &result_buffer.buffer, &result_buffer.allocation, &alloc_info_out);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create buffer");
        return result;
    }

    result_buffer.bufferSize = buffer_info.size;
    result_buffer.mapping    = static_cast<uint8_t*>(alloc_info_out.pMappedData);

    // Get the GPU address of the buffer
    const VkBufferDeviceAddressInfo info = { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = result_buffer.buffer };
    result_buffer.address                = vkGetBufferDeviceAddress(m_Device, &info);

    addLeakDetection(result_buffer.allocation);

    return result;
}

void vk_test::ResourceAllocator::destroyBuffer(vk_test::Buffer& buffer) const {
    vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
    buffer = {};
}

VkResult vk_test::ResourceAllocator::createLargeBuffer(LargeBuffer&                   large_buffer,
                                                       const VkBufferCreateInfo&      buffer_info,
                                                       const VmaAllocationCreateInfo& alloc_info,
                                                       VkQueue                        sparse_binding_queue,
                                                       VkFence                        sparse_binding_fence,
                                                       VkDeviceSize                   max_chunk_size,
                                                       VkDeviceSize                   min_alignment) const {
    assert(sparse_binding_queue);

    large_buffer = {};

    max_chunk_size = std::min(m_MaxMemoryAllocationSize, max_chunk_size);

    if (buffer_info.size <= max_chunk_size) {
        Buffer buffer;
        createBuffer(buffer, buffer_info, alloc_info, min_alignment);

        large_buffer.buffer      = buffer.buffer;
        large_buffer.bufferSize  = buffer.bufferSize;
        large_buffer.address     = buffer.address;
        large_buffer.allocations = { buffer.allocation };

        return VK_SUCCESS;
    }

    VkBufferCreateInfo create_info = buffer_info;

    create_info.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;

    vkCreateBuffer(m_Device, &create_info, nullptr, &large_buffer.buffer);

    // Find memory requirements
    VkMemoryRequirements2           mem_reqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkMemoryDedicatedRequirements   dedicated_regs{ VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
    VkBufferMemoryRequirementsInfo2 buffer_reqs{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };

    mem_reqs.pNext     = &dedicated_regs;
    buffer_reqs.buffer = large_buffer.buffer;

    vkGetBufferMemoryRequirements2(m_Device, &buffer_reqs, &mem_reqs);
    mem_reqs.memoryRequirements.alignment = std::max(min_alignment, mem_reqs.memoryRequirements.alignment);

    // align maxChunkSize to required alignment
    size_t page_alignment = mem_reqs.memoryRequirements.alignment;
    max_chunk_size        = (max_chunk_size + page_alignment - 1) & ~(page_alignment - 1);

    // get chunk count
    size_t full_chunk_count  = buffer_info.size / max_chunk_size;
    size_t total_chunk_count = (buffer_info.size + max_chunk_size - 1) / max_chunk_size;

    large_buffer.allocations.resize(total_chunk_count);
    std::vector<VmaAllocationInfo> allocation_infos(total_chunk_count);

    // full chunks first
    mem_reqs.memoryRequirements.size = max_chunk_size;
    VkResult result                  = vmaAllocateMemoryPages(m_Allocator, &mem_reqs.memoryRequirements, &alloc_info, full_chunk_count, large_buffer.allocations.data(), allocation_infos.data());
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_Device, large_buffer.buffer, nullptr);
        large_buffer = {};
        return result;
    }

    // tail chunk last
    if (full_chunk_count != total_chunk_count) {
        mem_reqs.memoryRequirements.size = create_info.size - full_chunk_count * max_chunk_size;
        mem_reqs.memoryRequirements.size = (mem_reqs.memoryRequirements.size + page_alignment - 1) & ~(page_alignment - 1);

        result = vmaAllocateMemoryPages(m_Allocator, &mem_reqs.memoryRequirements, &alloc_info, 1, large_buffer.allocations.data() + full_chunk_count, allocation_infos.data() + full_chunk_count);
        if (result != VK_SUCCESS) {
            vmaFreeMemoryPages(m_Allocator, full_chunk_count, large_buffer.allocations.data());
            vkDestroyBuffer(m_Device, large_buffer.buffer, nullptr);
            large_buffer = {};
            return result;
        }
    }

    std::vector<VkSparseMemoryBind> sparse_binds(total_chunk_count);

    for (uint32_t i = 0; i < total_chunk_count; i++) {
        VkSparseMemoryBind& sparse_bind = sparse_binds[i];
        sparse_bind.flags               = 0;
        sparse_bind.memory              = allocation_infos[i].deviceMemory;
        sparse_bind.memoryOffset        = allocation_infos[i].offset;
        sparse_bind.resourceOffset      = i * max_chunk_size;
        sparse_bind.size                = std::min(max_chunk_size, create_info.size - (i * max_chunk_size));
        sparse_bind.size                = (sparse_bind.size + page_alignment - 1) & ~(page_alignment - 1);

        addLeakDetection(large_buffer.allocations[i]);
    }

    VkSparseBufferMemoryBindInfo sparse_buffer_memory_bind_info{};
    sparse_buffer_memory_bind_info.buffer    = large_buffer.buffer;
    sparse_buffer_memory_bind_info.bindCount = uint32_t(sparse_binds.size());
    sparse_buffer_memory_bind_info.pBinds    = sparse_binds.data();

    VkBindSparseInfo bind_sparse_info{ VK_STRUCTURE_TYPE_BIND_SPARSE_INFO };
    bind_sparse_info.bufferBindCount = 1;
    bind_sparse_info.pBufferBinds    = &sparse_buffer_memory_bind_info;

    result = vkQueueBindSparse(sparse_binding_queue, 1, &bind_sparse_info, sparse_binding_fence);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_Device, large_buffer.buffer, nullptr);
        vmaFreeMemoryPages(m_Allocator, large_buffer.allocations.size(), large_buffer.allocations.data());
        large_buffer = {};
        return result;
    }

    if (sparse_binding_fence == nullptr) {
        result = vkQueueWaitIdle(sparse_binding_queue);
        if (result != VK_SUCCESS) {
            if (result != VK_ERROR_DEVICE_LOST) {
                vkDestroyBuffer(m_Device, large_buffer.buffer, nullptr);
                vmaFreeMemoryPages(m_Allocator, large_buffer.allocations.size(), large_buffer.allocations.data());
                large_buffer = {};
            }
            return result;
        }
    }

    // Get the GPU address of the buffer
    const VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = large_buffer.buffer,
    };
    large_buffer.address    = vkGetBufferDeviceAddress(m_Device, &info);
    large_buffer.bufferSize = create_info.size;

    return VK_SUCCESS;
}

VkResult vk_test::ResourceAllocator::createLargeBuffer(LargeBuffer&              large_buffer,
                                                       VkDeviceSize              size,
                                                       VkBufferUsageFlags2KHR    usage,
                                                       VkQueue                   sparse_binding_queue,
                                                       VkFence                   sparse_binding_fence /*= VK_NULL_HANDLE*/,
                                                       VkDeviceSize              max_chunk_size /*= DEFAULT_LARGE_CHUNK_SIZE*/,
                                                       VmaMemoryUsage            memory_usage /*= VMA_MEMORY_USAGE_AUTO*/,
                                                       VmaAllocationCreateFlags  flags /*= {}*/,
                                                       VkDeviceSize              min_alignment /*= 0*/,
                                                       std::span<const uint32_t> queue_families /*= {}*/) const {
    const VkBufferUsageFlags2CreateInfo buffer_usage_flags2_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = usage | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
    };

    const VkBufferCreateInfo buffer_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &buffer_usage_flags2_create_info,
        .size                  = size,
        .usage                 = 0,
        .sharingMode           = queue_families.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size()),
        .pQueueFamilyIndices   = queue_families.data(),
    };

    VmaAllocationCreateInfo alloc_info        = { .flags = flags, .usage = memory_usage };
    uint32_t                memory_type_index = 0;
    vmaFindMemoryTypeIndexForBufferInfo(m_Allocator, &buffer_info, &alloc_info, &memory_type_index);

    alloc_info.usage          = VMA_MEMORY_USAGE_UNKNOWN;
    alloc_info.memoryTypeBits = 1 << memory_type_index;

    return createLargeBuffer(large_buffer, buffer_info, alloc_info, sparse_binding_queue, sparse_binding_fence, max_chunk_size, min_alignment);
}

void vk_test::ResourceAllocator::destroyLargeBuffer(LargeBuffer& buffer) const {
    vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
    vmaFreeMemoryPages(m_Allocator, buffer.allocations.size(), buffer.allocations.data());
    buffer = {};
}

VkResult vk_test::ResourceAllocator::createImage(vk_test::Image& image, const VkImageCreateInfo& image_info, const VmaAllocationCreateInfo& vma_info) const {
    image = {};

    VmaAllocationInfo alloc_info_out{};
    VkResult          result = vmaCreateImage(m_Allocator, &image_info, &vma_info, &image.image, &image.allocation, &alloc_info_out);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create image\n");
    }

    image.extent                 = image_info.extent;
    image.mip_levels             = image_info.mipLevels;
    image.array_layers           = image_info.arrayLayers;
    image.format                 = image_info.format;
    image.descriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    addLeakDetection(image.allocation);

    return result;
}

VkResult vk_test::ResourceAllocator::createImage(vk_test::Image& image, const VkImageCreateInfo& image_info) const {
    const VmaAllocationCreateInfo alloc_info{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createImage(image, image_info, alloc_info);
}

VkResult vk_test::ResourceAllocator::createImage(Image& image, const VkImageCreateInfo& image_info, const VkImageViewCreateInfo& image_view_info) const {
    const VmaAllocationCreateInfo alloc_info{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createImage(image, image_info, image_view_info, alloc_info);
}

VkResult vk_test::ResourceAllocator::createImage(Image&                         image,
                                                 const VkImageCreateInfo&       image_info,
                                                 const VkImageViewCreateInfo&   image_view_info,
                                                 const VmaAllocationCreateInfo& vma_info) const {
    VkResult result{};
    // Create image in GPU memory
    VkImageCreateInfo copy_image_info = image_info;
    copy_image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // We will copy data to this image
    result = createImage(image, copy_image_info, vma_info);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create image view
    VkImageViewCreateInfo view_info = image_view_info;
    view_info.image                 = image.image;
    view_info.format                = image_info.format;
    vkCreateImageView(m_Device, &view_info, nullptr, &image.descriptor.imageView);

    return VK_SUCCESS;
}

void vk_test::ResourceAllocator::destroyImage(Image& image) const {
    vkDestroyImageView(m_Device, image.descriptor.imageView, nullptr);
    vmaDestroyImage(m_Allocator, image.image, image.allocation);
    image = {};
}

VkResult vk_test::ResourceAllocator::createAcceleration(vk_test::AccelerationStructure&             result_accel,
                                                        const VkAccelerationStructureCreateInfoKHR& acc_info,
                                                        const VmaAllocationCreateInfo&              vma_info,
                                                        std::span<const uint32_t>                   queue_families) const {
    result_accel                                      = {};
    VkAccelerationStructureCreateInfoKHR accel_struct = acc_info;

    const VkBufferUsageFlags2CreateInfo buffer_usage_flags2_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
    };

    const VkBufferCreateInfo buffer_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &buffer_usage_flags2_create_info,
        .size                  = accel_struct.size,
        .usage                 = 0,
        .sharingMode           = queue_families.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size()),
        .pQueueFamilyIndices   = queue_families.data(),
    };

    // Step 1: Create the buffer to hold the acceleration structure
    VkResult result = createBuffer(result_accel.buffer, buffer_info, vma_info);

    if (result != VK_SUCCESS) {
        return result;
    }

    // Step 2: Create the acceleration structure with the buffer
    accel_struct.buffer = result_accel.buffer.buffer;
    result              = vkCreateAccelerationStructureKHR(m_Device, &accel_struct, nullptr, &result_accel.accel);

    if (result != VK_SUCCESS) {
        destroyBuffer(result_accel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = result_accel.accel };
        result_accel.address = vkGetAccelerationStructureDeviceAddressKHR(m_Device, &info);
    }

    return result;
}

VkResult vk_test::ResourceAllocator::createAcceleration(vk_test::AccelerationStructure&             accel,
                                                        const VkAccelerationStructureCreateInfoKHR& acc_info) const {
    const VmaAllocationCreateInfo alloc_info{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createAcceleration(accel, acc_info, alloc_info);
}

void vk_test::ResourceAllocator::destroyAcceleration(vk_test::AccelerationStructure& accel) const {
    destroyBuffer(accel.buffer);
    vkDestroyAccelerationStructureKHR(m_Device, accel.accel, nullptr);
    accel = {};
}

VkResult vk_test::ResourceAllocator::createLargeAcceleration(LargeAccelerationStructure&                 result_accel,
                                                             const VkAccelerationStructureCreateInfoKHR& acc_info,
                                                             const VmaAllocationCreateInfo&              vma_info,
                                                             VkQueue                                     sparse_binding_queue,
                                                             VkFence                                     sparse_binding_fence,
                                                             VkDeviceSize                                max_chunk_size,
                                                             std::span<const uint32_t>                   queue_families) const {
    result_accel                                      = {};
    VkAccelerationStructureCreateInfoKHR accel_struct = acc_info;

    const VkBufferUsageFlags2CreateInfo buffer_usage_flags2_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR,
    };

    const VkBufferCreateInfo buffer_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &buffer_usage_flags2_create_info,
        .flags                 = VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
        .size                  = accel_struct.size,
        .usage                 = 0,
        .sharingMode           = queue_families.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size()),
        .pQueueFamilyIndices   = queue_families.data(),
    };

    // Step 1: Create the buffer to hold the acceleration structure
    VkResult result = createLargeBuffer(result_accel.buffer, buffer_info, vma_info, sparse_binding_queue, sparse_binding_fence, max_chunk_size);

    if (result != VK_SUCCESS) {
        return result;
    }

    // Step 2: Create the acceleration structure with the buffer
    accel_struct.buffer = result_accel.buffer.buffer;
    result              = vkCreateAccelerationStructureKHR(m_Device, &accel_struct, nullptr, &result_accel.accel);

    if (result != VK_SUCCESS) {
        destroyLargeBuffer(result_accel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = result_accel.accel };
        result_accel.address = vkGetAccelerationStructureDeviceAddressKHR(m_Device, &info);
    }

    return result;
}

VkResult vk_test::ResourceAllocator::createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                                             const VkAccelerationStructureCreateInfoKHR& acc_info,
                                                             VkQueue                                     sparse_binding_queue,
                                                             VkFence                                     sparse_binding_fence,
                                                             VkDeviceSize                                max_chunk_size) const {
    VmaAllocationCreateInfo alloc_info = { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createLargeAcceleration(accel, acc_info, alloc_info, sparse_binding_queue, sparse_binding_fence, max_chunk_size);
}

void vk_test::ResourceAllocator::destroyLargeAcceleration(LargeAccelerationStructure& accel) const {
    vkDestroyAccelerationStructureKHR(m_Device, accel.accel, nullptr);
    destroyLargeBuffer(accel.buffer);

    accel = {};
}

void vk_test::ResourceAllocator::setLeakID(uint32_t id) {
    m_LeakID = id;
}

VkDeviceMemory vk_test::ResourceAllocator::getDeviceMemory(VmaAllocation allocation) const {
    VmaAllocationInfo allocation_info;
    vmaGetAllocationInfo(*this, allocation, &allocation_info);
    return allocation_info.deviceMemory;
}

VkResult vk_test::ResourceAllocator::flushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    return vmaFlushAllocation(m_Allocator, buffer.allocation, offset, size);
}

VkResult vk_test::ResourceAllocator::invalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    return vmaInvalidateAllocation(m_Allocator, buffer.allocation, offset, size);
}

VkResult vk_test::ResourceAllocator::autoFlushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    VkMemoryPropertyFlags mem_flags{};
    vmaGetAllocationMemoryProperties(m_Allocator, buffer.allocation, &mem_flags);
    if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0U) {
        return vmaFlushAllocation(m_Allocator, buffer.allocation, offset, size);
    }

    return VK_SUCCESS;
}

VkResult vk_test::ResourceAllocator::autoInvalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    VkMemoryPropertyFlags mem_flags{};
    vmaGetAllocationMemoryProperties(m_Allocator, buffer.allocation, &mem_flags);
    if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0U) {
        return vmaInvalidateAllocation(m_Allocator, buffer.allocation, offset, size);
    }

    return VK_SUCCESS;
}
