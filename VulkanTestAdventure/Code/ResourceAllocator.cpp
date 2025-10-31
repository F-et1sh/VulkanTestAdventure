#include "pch.h"
#include "ResourceAllocator.hpp"

vk_test::ResourceAllocator::ResourceAllocator(ResourceAllocator&& other) noexcept {
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_device, other.m_device);
    std::swap(m_physicalDevice, other.m_physicalDevice);
    std::swap(m_leakID, other.m_leakID);
    std::swap(m_maxMemoryAllocationSize, other.m_maxMemoryAllocationSize);
}

vk_test::ResourceAllocator& vk_test::ResourceAllocator::operator=(ResourceAllocator&& other) noexcept {
    if (this != &other) {
        assert(m_allocator == nullptr && "Missing deinit()");

        std::swap(m_allocator, other.m_allocator);
        std::swap(m_device, other.m_device);
        std::swap(m_physicalDevice, other.m_physicalDevice);
        std::swap(m_leakID, other.m_leakID);
        std::swap(m_maxMemoryAllocationSize, other.m_maxMemoryAllocationSize);
    }

    return *this;
}

vk_test::ResourceAllocator::~ResourceAllocator() {
    assert(m_allocator == nullptr && "Missing deinit()");
}

vk_test::ResourceAllocator::operator VmaAllocator() const {
    return m_allocator;
}

VkResult vk_test::ResourceAllocator::init(VmaAllocatorCreateInfo allocator_info) {
    assert(m_allocator == nullptr);

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
    m_maxMemoryAllocationSize = props11.maxMemoryAllocationSize;

    VK_TEST_SAY("Max size : " << m_maxMemoryAllocationSize);

    m_device         = allocator_info.device;
    m_physicalDevice = allocator_info.physicalDevice;

    // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };
    allocator_info.pVulkanFunctions = &functions;
    return vmaCreateAllocator(&allocator_info, &m_allocator);
}

void vk_test::ResourceAllocator::deinit() {
    if (m_allocator == nullptr) {
        return;
    }

    vmaDestroyAllocator(m_allocator);
    m_allocator      = nullptr;
    m_device         = nullptr;
    m_physicalDevice = nullptr;
    m_leakID         = ~0;
}

void vk_test::ResourceAllocator::addLeakDetection(VmaAllocation allocation) const {
    /*if(m_leakID == m_allocationCounter)
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
  vmaSetAllocationName(m_allocator, allocation, nvvkAllocID.c_str());*/
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

    VkResult result = vmaCreateBufferWithAlignment(m_allocator, &buffer_info, &alloc_info, min_alignment, &result_buffer.buffer, &result_buffer.allocation, &alloc_info_out);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create buffer");
        return result;
    }

    result_buffer.bufferSize = buffer_info.size;
    result_buffer.mapping    = static_cast<uint8_t*>(alloc_info_out.pMappedData);

    // Get the GPU address of the buffer
    const VkBufferDeviceAddressInfo info = { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = result_buffer.buffer };
    result_buffer.address                = vkGetBufferDeviceAddress(m_device, &info);

    addLeakDetection(result_buffer.allocation);

    return result;
}

void vk_test::ResourceAllocator::destroyBuffer(vk_test::Buffer& buffer) const {
    vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    buffer = {};
}

VkResult vk_test::ResourceAllocator::createLargeBuffer(LargeBuffer&                   largeBuffer,
                                                       const VkBufferCreateInfo&      bufferInfo,
                                                       const VmaAllocationCreateInfo& allocInfo,
                                                       VkQueue                        sparseBindingQueue,
                                                       VkFence                        sparseBindingFence,
                                                       VkDeviceSize                   maxChunkSize,
                                                       VkDeviceSize                   minAlignment) const {
    assert(sparseBindingQueue);

    largeBuffer = {};

    maxChunkSize = std::min(m_maxMemoryAllocationSize, maxChunkSize);

    if (bufferInfo.size <= maxChunkSize) {
        Buffer buffer;
        createBuffer(buffer, bufferInfo, allocInfo, minAlignment);

        largeBuffer.buffer      = buffer.buffer;
        largeBuffer.bufferSize  = buffer.bufferSize;
        largeBuffer.address     = buffer.address;
        largeBuffer.allocations = { buffer.allocation };

        return VK_SUCCESS;
    }

    VkBufferCreateInfo createInfo = bufferInfo;

    createInfo.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;

    vkCreateBuffer(m_device, &createInfo, nullptr, &largeBuffer.buffer);

    // Find memory requirements
    VkMemoryRequirements2           memReqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkMemoryDedicatedRequirements   dedicatedRegs{ VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
    VkBufferMemoryRequirementsInfo2 bufferReqs{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };

    memReqs.pNext     = &dedicatedRegs;
    bufferReqs.buffer = largeBuffer.buffer;

    vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);
    memReqs.memoryRequirements.alignment = std::max(minAlignment, memReqs.memoryRequirements.alignment);

    // align maxChunkSize to required alignment
    size_t pageAlignment = memReqs.memoryRequirements.alignment;
    maxChunkSize         = (maxChunkSize + pageAlignment - 1) & ~(pageAlignment - 1);

    // get chunk count
    size_t fullChunkCount  = bufferInfo.size / maxChunkSize;
    size_t totalChunkCount = (bufferInfo.size + maxChunkSize - 1) / maxChunkSize;

    largeBuffer.allocations.resize(totalChunkCount);
    std::vector<VmaAllocationInfo> allocationInfos(totalChunkCount);

    // full chunks first
    memReqs.memoryRequirements.size = maxChunkSize;
    VkResult result                 = vmaAllocateMemoryPages(m_allocator, &memReqs.memoryRequirements, &allocInfo, fullChunkCount, largeBuffer.allocations.data(), allocationInfos.data());
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_device, largeBuffer.buffer, nullptr);
        largeBuffer = {};
        return result;
    }

    // tail chunk last
    if (fullChunkCount != totalChunkCount) {
        memReqs.memoryRequirements.size = createInfo.size - fullChunkCount * maxChunkSize;
        memReqs.memoryRequirements.size = (memReqs.memoryRequirements.size + pageAlignment - 1) & ~(pageAlignment - 1);

        result = vmaAllocateMemoryPages(m_allocator, &memReqs.memoryRequirements, &allocInfo, 1, largeBuffer.allocations.data() + fullChunkCount, allocationInfos.data() + fullChunkCount);
        if (result != VK_SUCCESS) {
            vmaFreeMemoryPages(m_allocator, fullChunkCount, largeBuffer.allocations.data());
            vkDestroyBuffer(m_device, largeBuffer.buffer, nullptr);
            largeBuffer = {};
            return result;
        }
    }

    std::vector<VkSparseMemoryBind> sparseBinds(totalChunkCount);

    for (uint32_t i = 0; i < totalChunkCount; i++) {
        VkSparseMemoryBind& sparseBind = sparseBinds[i];
        sparseBind.flags               = 0;
        sparseBind.memory              = allocationInfos[i].deviceMemory;
        sparseBind.memoryOffset        = allocationInfos[i].offset;
        sparseBind.resourceOffset      = i * maxChunkSize;
        sparseBind.size                = std::min(maxChunkSize, createInfo.size - i * maxChunkSize);
        sparseBind.size                = (sparseBind.size + pageAlignment - 1) & ~(pageAlignment - 1);

        addLeakDetection(largeBuffer.allocations[i]);
    }

    VkSparseBufferMemoryBindInfo sparseBufferMemoryBindInfo{};
    sparseBufferMemoryBindInfo.buffer    = largeBuffer.buffer;
    sparseBufferMemoryBindInfo.bindCount = uint32_t(sparseBinds.size());
    sparseBufferMemoryBindInfo.pBinds    = sparseBinds.data();

    VkBindSparseInfo bindSparseInfo{ VK_STRUCTURE_TYPE_BIND_SPARSE_INFO };
    bindSparseInfo.bufferBindCount = 1;
    bindSparseInfo.pBufferBinds    = &sparseBufferMemoryBindInfo;

    result = vkQueueBindSparse(sparseBindingQueue, 1, &bindSparseInfo, sparseBindingFence);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_device, largeBuffer.buffer, nullptr);
        vmaFreeMemoryPages(m_allocator, largeBuffer.allocations.size(), largeBuffer.allocations.data());
        largeBuffer = {};
        return result;
    }

    if (!sparseBindingFence) {
        result = vkQueueWaitIdle(sparseBindingQueue);
        if (result != VK_SUCCESS) {
            if (result != VK_ERROR_DEVICE_LOST) {
                vkDestroyBuffer(m_device, largeBuffer.buffer, nullptr);
                vmaFreeMemoryPages(m_allocator, largeBuffer.allocations.size(), largeBuffer.allocations.data());
                largeBuffer = {};
            }
            return result;
        }
    }

    // Get the GPU address of the buffer
    const VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = largeBuffer.buffer,
    };
    largeBuffer.address    = vkGetBufferDeviceAddress(m_device, &info);
    largeBuffer.bufferSize = createInfo.size;

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
    vmaFindMemoryTypeIndexForBufferInfo(m_allocator, &buffer_info, &alloc_info, &memory_type_index);

    alloc_info.usage          = VMA_MEMORY_USAGE_UNKNOWN;
    alloc_info.memoryTypeBits = 1 << memory_type_index;

    return createLargeBuffer(large_buffer, buffer_info, alloc_info, sparse_binding_queue, sparse_binding_fence, max_chunk_size, min_alignment);
}

void vk_test::ResourceAllocator::destroyLargeBuffer(LargeBuffer& buffer) const {
    vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    vmaFreeMemoryPages(m_allocator, buffer.allocations.size(), buffer.allocations.data());
    buffer = {};
}

VkResult vk_test::ResourceAllocator::createImage(vk_test::Image& image, const VkImageCreateInfo& image_info, const VmaAllocationCreateInfo& vma_info) const {
    image = {};

    VmaAllocationInfo alloc_info_out{};
    VkResult          result = vmaCreateImage(m_allocator, &image_info, &vma_info, &image.image, &image.allocation, &alloc_info_out);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create image\n");
    }

    image.extent                 = image_info.extent;
    image.mipLevels              = image_info.mipLevels;
    image.arrayLayers            = image_info.arrayLayers;
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
    vkCreateImageView(m_device, &view_info, nullptr, &image.descriptor.imageView);

    return VK_SUCCESS;
}

void vk_test::ResourceAllocator::destroyImage(Image& image) const {
    vkDestroyImageView(m_device, image.descriptor.imageView, nullptr);
    vmaDestroyImage(m_allocator, image.image, image.allocation);
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
    result              = vkCreateAccelerationStructureKHR(m_device, &accel_struct, nullptr, &result_accel.accel);

    if (result != VK_SUCCESS) {
        destroyBuffer(result_accel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = result_accel.accel };
        result_accel.address = vkGetAccelerationStructureDeviceAddressKHR(m_device, &info);
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
    vkDestroyAccelerationStructureKHR(m_device, accel.accel, nullptr);
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
    result              = vkCreateAccelerationStructureKHR(m_device, &accel_struct, nullptr, &result_accel.accel);

    if (result != VK_SUCCESS) {
        destroyLargeBuffer(result_accel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = result_accel.accel };
        result_accel.address = vkGetAccelerationStructureDeviceAddressKHR(m_device, &info);
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
    vkDestroyAccelerationStructureKHR(m_device, accel.accel, nullptr);
    destroyLargeBuffer(accel.buffer);

    accel = {};
}

void vk_test::ResourceAllocator::setLeakID(uint32_t id) {
    m_leakID = id;
}

VkDeviceMemory vk_test::ResourceAllocator::getDeviceMemory(VmaAllocation allocation) const {
    VmaAllocationInfo allocation_info;
    vmaGetAllocationInfo(*this, allocation, &allocation_info);
    return allocation_info.deviceMemory;
}

VkResult vk_test::ResourceAllocator::flushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    return vmaFlushAllocation(m_allocator, buffer.allocation, offset, size);
}

VkResult vk_test::ResourceAllocator::invalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    return vmaInvalidateAllocation(m_allocator, buffer.allocation, offset, size);
}

VkResult vk_test::ResourceAllocator::autoFlushBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    VkMemoryPropertyFlags mem_flags{};
    vmaGetAllocationMemoryProperties(m_allocator, buffer.allocation, &mem_flags);
    if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0u) {
        return vmaFlushAllocation(m_allocator, buffer.allocation, offset, size);
    }

    return VK_SUCCESS;
}

VkResult vk_test::ResourceAllocator::autoInvalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    VkMemoryPropertyFlags mem_flags{};
    vmaGetAllocationMemoryProperties(m_allocator, buffer.allocation, &mem_flags);
    if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0u) {
        return vmaInvalidateAllocation(m_allocator, buffer.allocation, offset, size);
    }

    return VK_SUCCESS;
}
