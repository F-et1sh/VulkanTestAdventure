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

VkResult vk_test::ResourceAllocator::init(VmaAllocatorCreateInfo allocatorInfo) {
    assert(m_allocator == nullptr);

    // #TODO : VK_EXT_memory_priority ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT

    allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // allow querying for the GPU address of a buffer
    allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT; // allow using VkBufferUsageFlags2CreateInfoKHR

    VkPhysicalDeviceVulkan11Properties props11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
    };

    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &props11,
    };
    vkGetPhysicalDeviceProperties2(allocatorInfo.physicalDevice, &props);
    m_maxMemoryAllocationSize = props11.maxMemoryAllocationSize;

    VK_TEST_SAY("Max size : " << m_maxMemoryAllocationSize);

    m_device         = allocatorInfo.device;
    m_physicalDevice = allocatorInfo.physicalDevice;

    // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };
    allocatorInfo.pVulkanFunctions = &functions;
    return vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

void vk_test::ResourceAllocator::deinit() {
    if (!m_allocator)
        return;

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
                                                  VmaMemoryUsage            memoryUsage,
                                                  VmaAllocationCreateFlags  flags,
                                                  VkDeviceSize              minAlignment,
                                                  std::span<const uint32_t> queueFamilies) const {
    const VkBufferUsageFlags2CreateInfo bufferUsageFlags2CreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = usage | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
    };

    const VkBufferCreateInfo bufferInfo{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufferUsageFlags2CreateInfo,
        .size                  = size,
        .usage                 = 0,
        .sharingMode           = queueFamilies.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size()),
        .pQueueFamilyIndices   = queueFamilies.data(),
    };

    VmaAllocationCreateInfo allocInfo = { .flags = flags, .usage = memoryUsage };

    return createBuffer(buffer, bufferInfo, allocInfo, minAlignment);
}

VkResult vk_test::ResourceAllocator::createBuffer(vk_test::Buffer&               resultBuffer,
                                                  const VkBufferCreateInfo&      bufferInfo,
                                                  const VmaAllocationCreateInfo& allocInfo,
                                                  VkDeviceSize                   minAlignment) const {
    resultBuffer = {};

    // Create the buffer
    VmaAllocationInfo allocInfoOut{};

    VkResult result = vmaCreateBufferWithAlignment(m_allocator, &bufferInfo, &allocInfo, minAlignment, &resultBuffer.buffer, &resultBuffer.allocation, &allocInfoOut);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create buffer");
        return result;
    }

    resultBuffer.bufferSize = bufferInfo.size;
    resultBuffer.mapping    = static_cast<uint8_t*>(allocInfoOut.pMappedData);

    // Get the GPU address of the buffer
    const VkBufferDeviceAddressInfo info = { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = resultBuffer.buffer };
    resultBuffer.address                 = vkGetBufferDeviceAddress(m_device, &info);

    addLeakDetection(resultBuffer.allocation);

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
    else {
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
}

VkResult vk_test::ResourceAllocator::createLargeBuffer(LargeBuffer&              largeBuffer,
                                                       VkDeviceSize              size,
                                                       VkBufferUsageFlags2KHR    usage,
                                                       VkQueue                   sparseBindingQueue,
                                                       VkFence                   sparseBindingFence /*= VK_NULL_HANDLE*/,
                                                       VkDeviceSize              maxChunkSize /*= DEFAULT_LARGE_CHUNK_SIZE*/,
                                                       VmaMemoryUsage            memoryUsage /*= VMA_MEMORY_USAGE_AUTO*/,
                                                       VmaAllocationCreateFlags  flags /*= {}*/,
                                                       VkDeviceSize              minAlignment /*= 0*/,
                                                       std::span<const uint32_t> queueFamilies /*= {}*/) const {
    const VkBufferUsageFlags2CreateInfo bufferUsageFlags2CreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = usage | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
    };

    const VkBufferCreateInfo bufferInfo{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufferUsageFlags2CreateInfo,
        .size                  = size,
        .usage                 = 0,
        .sharingMode           = queueFamilies.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size()),
        .pQueueFamilyIndices   = queueFamilies.data(),
    };

    VmaAllocationCreateInfo allocInfo       = { .flags = flags, .usage = memoryUsage };
    uint32_t                memoryTypeIndex = 0;
    vmaFindMemoryTypeIndexForBufferInfo(m_allocator, &bufferInfo, &allocInfo, &memoryTypeIndex);

    allocInfo.usage          = VMA_MEMORY_USAGE_UNKNOWN;
    allocInfo.memoryTypeBits = 1 << memoryTypeIndex;

    return createLargeBuffer(largeBuffer, bufferInfo, allocInfo, sparseBindingQueue, sparseBindingFence, maxChunkSize, minAlignment);
}

void vk_test::ResourceAllocator::destroyLargeBuffer(LargeBuffer& buffer) const {
    vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    vmaFreeMemoryPages(m_allocator, buffer.allocations.size(), buffer.allocations.data());
    buffer = {};
}

VkResult vk_test::ResourceAllocator::createImage(vk_test::Image& image, const VkImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo) const {
    image = {};

    VmaAllocationInfo allocInfoOut{};
    VkResult          result = vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &image.image, &image.allocation, &allocInfoOut);

    if (result != VK_SUCCESS) {
        // Handle allocation failure
        VK_TEST_SAY("Failed to create image\n");
    }

    image.extent                 = imageInfo.extent;
    image.mipLevels              = imageInfo.mipLevels;
    image.arrayLayers            = imageInfo.arrayLayers;
    image.format                 = imageInfo.format;
    image.descriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    addLeakDetection(image.allocation);

    return result;
}

VkResult vk_test::ResourceAllocator::createImage(vk_test::Image& image, const VkImageCreateInfo& imageInfo) const {
    const VmaAllocationCreateInfo allocInfo{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createImage(image, imageInfo, allocInfo);
}

VkResult vk_test::ResourceAllocator::createImage(Image& image, const VkImageCreateInfo& imageInfo, const VkImageViewCreateInfo& imageViewInfo) const {
    const VmaAllocationCreateInfo allocInfo{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createImage(image, imageInfo, imageViewInfo, allocInfo);
}

VkResult vk_test::ResourceAllocator::createImage(Image&                         image,
                                                 const VkImageCreateInfo&       _imageInfo,
                                                 const VkImageViewCreateInfo&   _imageViewInfo,
                                                 const VmaAllocationCreateInfo& vmaInfo) const {
    VkResult result{};
    // Create image in GPU memory
    VkImageCreateInfo imageInfo = _imageInfo;
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // We will copy data to this image
    result = createImage(image, imageInfo, vmaInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create image view
    VkImageViewCreateInfo viewInfo = _imageViewInfo;
    viewInfo.image                 = image.image;
    viewInfo.format                = _imageInfo.format;
    vkCreateImageView(m_device, &viewInfo, nullptr, &image.descriptor.imageView);

    return VK_SUCCESS;
}

void vk_test::ResourceAllocator::destroyImage(Image& image) const {
    vkDestroyImageView(m_device, image.descriptor.imageView, nullptr);
    vmaDestroyImage(m_allocator, image.image, image.allocation);
    image = {};
}

VkResult vk_test::ResourceAllocator::createAcceleration(vk_test::AccelerationStructure&             resultAccel,
                                                        const VkAccelerationStructureCreateInfoKHR& accInfo,
                                                        const VmaAllocationCreateInfo&              vmaInfo,
                                                        std::span<const uint32_t>                   queueFamilies) const {
    resultAccel                                      = {};
    VkAccelerationStructureCreateInfoKHR accelStruct = accInfo;

    const VkBufferUsageFlags2CreateInfo bufferUsageFlags2CreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
    };

    const VkBufferCreateInfo bufferInfo{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufferUsageFlags2CreateInfo,
        .size                  = accelStruct.size,
        .usage                 = 0,
        .sharingMode           = queueFamilies.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size()),
        .pQueueFamilyIndices   = queueFamilies.data(),
    };

    // Step 1: Create the buffer to hold the acceleration structure
    VkResult result = createBuffer(resultAccel.buffer, bufferInfo, vmaInfo);

    if (result != VK_SUCCESS) {
        return result;
    }

    // Step 2: Create the acceleration structure with the buffer
    accelStruct.buffer = resultAccel.buffer.buffer;
    result             = vkCreateAccelerationStructureKHR(m_device, &accelStruct, nullptr, &resultAccel.accel);

    if (result != VK_SUCCESS) {
        destroyBuffer(resultAccel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = resultAccel.accel };
        resultAccel.address = vkGetAccelerationStructureDeviceAddressKHR(m_device, &info);
    }

    return result;
}

VkResult vk_test::ResourceAllocator::createAcceleration(vk_test::AccelerationStructure&             accel,
                                                        const VkAccelerationStructureCreateInfoKHR& inAccInfo) const {
    const VmaAllocationCreateInfo allocInfo{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createAcceleration(accel, inAccInfo, allocInfo);
}

void vk_test::ResourceAllocator::destroyAcceleration(vk_test::AccelerationStructure& accel) const {
    destroyBuffer(accel.buffer);
    vkDestroyAccelerationStructureKHR(m_device, accel.accel, nullptr);
    accel = {};
}

VkResult vk_test::ResourceAllocator::createLargeAcceleration(LargeAccelerationStructure&                 resultAccel,
                                                             const VkAccelerationStructureCreateInfoKHR& accInfo,
                                                             const VmaAllocationCreateInfo&              vmaInfo,
                                                             VkQueue                                     sparseBindingQueue,
                                                             VkFence                                     sparseBindingFence,
                                                             VkDeviceSize                                maxChunkSize,
                                                             std::span<const uint32_t>                   queueFamilies) const {
    resultAccel                                      = {};
    VkAccelerationStructureCreateInfoKHR accelStruct = accInfo;

    const VkBufferUsageFlags2CreateInfo bufferUsageFlags2CreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR,
    };

    const VkBufferCreateInfo bufferInfo{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufferUsageFlags2CreateInfo,
        .flags                 = VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
        .size                  = accelStruct.size,
        .usage                 = 0,
        .sharingMode           = queueFamilies.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size()),
        .pQueueFamilyIndices   = queueFamilies.data(),
    };

    // Step 1: Create the buffer to hold the acceleration structure
    VkResult result = createLargeBuffer(resultAccel.buffer, bufferInfo, vmaInfo, sparseBindingQueue, sparseBindingFence, maxChunkSize);

    if (result != VK_SUCCESS) {
        return result;
    }

    // Step 2: Create the acceleration structure with the buffer
    accelStruct.buffer = resultAccel.buffer.buffer;
    result             = vkCreateAccelerationStructureKHR(m_device, &accelStruct, nullptr, &resultAccel.accel);

    if (result != VK_SUCCESS) {
        destroyLargeBuffer(resultAccel.buffer);
        VK_TEST_SAY("Failed to create acceleration structure");
        return result;
    }

    {
        VkAccelerationStructureDeviceAddressInfoKHR info{ .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                          .accelerationStructure = resultAccel.accel };
        resultAccel.address = vkGetAccelerationStructureDeviceAddressKHR(m_device, &info);
    }

    return result;
}

VkResult vk_test::ResourceAllocator::createLargeAcceleration(LargeAccelerationStructure&                 accel,
                                                             const VkAccelerationStructureCreateInfoKHR& accInfo,
                                                             VkQueue                                     sparseBindingQueue,
                                                             VkFence                                     sparseBindingFence,
                                                             VkDeviceSize                                maxChunkSize) const {
    VmaAllocationCreateInfo allocInfo = { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

    return createLargeAcceleration(accel, accInfo, allocInfo, sparseBindingQueue, sparseBindingFence, maxChunkSize);
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
    VmaAllocationInfo allocationInfo;
    vmaGetAllocationInfo(*this, allocation, &allocationInfo);
    return allocationInfo.deviceMemory;
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
    VkMemoryPropertyFlags memFlags{};
    vmaGetAllocationMemoryProperties(m_allocator, buffer.allocation, &memFlags);
    if (!(memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return vmaFlushAllocation(m_allocator, buffer.allocation, offset, size);
    }
    else {
        return VK_SUCCESS;
    }
}

VkResult vk_test::ResourceAllocator::autoInvalidateBuffer(const vk_test::Buffer& buffer, VkDeviceSize offset /*= 0*/, VkDeviceSize size /*= VK_WHOLE_SIZE*/) {
    assert(buffer.mapping);
    VkMemoryPropertyFlags memFlags{};
    vmaGetAllocationMemoryProperties(m_allocator, buffer.allocation, &memFlags);
    if (!(memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return vmaInvalidateAllocation(m_allocator, buffer.allocation, offset, size);
    }
    else {
        return VK_SUCCESS;
    }
}
