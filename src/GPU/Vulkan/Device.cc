#include "Microcosm/GPU/Vulkan/Device"

namespace mi::vk {

static bool is_usable(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  for (uint32_t index = 0; index < count; index++) {
    VkBool32 supported = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supported);
    if (result == VK_SUCCESS and supported == VK_TRUE) return true;
  }
  return false;
}

static VkPhysicalDevice select_better(VkPhysicalDevice device0, VkPhysicalDevice device1) noexcept {
  if (device0 == VK_NULL_HANDLE) return device1;
  if (device1 == VK_NULL_HANDLE) return device0;

  VkPhysicalDeviceProperties props0;
  VkPhysicalDeviceProperties props1;
  vkGetPhysicalDeviceProperties(device0, &props0);
  vkGetPhysicalDeviceProperties(device1, &props1);
  // Compare device types.
  if (props0.deviceType != props1.deviceType) {
    // Prefer discrete GPUs first.
    if (props0.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return device0;
    if (props1.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return device1;
    // Prefer integrated GPUs second.
    if (props0.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return device0;
    if (props1.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return device1;
  }
  // Define helpers.
  int score0 = 0;
  int score1 = 0;
#define FavorMinimum(Limit)                              \
  score0 += (props0.limits.Limit < props1.limits.Limit); \
  score1 += (props0.limits.Limit > props1.limits.Limit)
#define FavorMaximum(Limit)                              \
  score0 += (props0.limits.Limit > props1.limits.Limit); \
  score1 += (props0.limits.Limit < props1.limits.Limit)
  {
    FavorMaximum(maxImageDimension1D);
    FavorMaximum(maxImageDimension2D);
    FavorMaximum(maxImageDimension3D);
    FavorMaximum(maxImageDimensionCube);
    FavorMaximum(maxImageArrayLayers);
    FavorMaximum(maxTexelBufferElements);
    FavorMaximum(maxUniformBufferRange);
    FavorMaximum(maxPushConstantsSize);
    FavorMaximum(maxMemoryAllocationCount);
    FavorMaximum(maxSamplerAllocationCount);
    FavorMinimum(bufferImageGranularity);
    FavorMaximum(sparseAddressSpaceSize);
    FavorMaximum(maxBoundDescriptorSets);
    FavorMaximum(maxPerStageDescriptorSamplers);
    FavorMaximum(maxPerStageDescriptorUniformBuffers);
    FavorMaximum(maxPerStageDescriptorStorageBuffers);
    FavorMaximum(maxPerStageDescriptorSampledImages);
    FavorMaximum(maxPerStageDescriptorStorageImages);
    FavorMaximum(maxPerStageDescriptorInputAttachments);
    FavorMaximum(maxPerStageResources);
    FavorMaximum(maxDescriptorSetSamplers);
    FavorMaximum(maxDescriptorSetUniformBuffers);
    FavorMaximum(maxDescriptorSetUniformBuffersDynamic);
    FavorMaximum(maxDescriptorSetStorageBuffers);
    FavorMaximum(maxDescriptorSetStorageBuffersDynamic);
    FavorMaximum(maxDescriptorSetSampledImages);
    FavorMaximum(maxDescriptorSetStorageImages);
    FavorMaximum(maxDescriptorSetInputAttachments);
    FavorMaximum(maxVertexInputAttributes);
    FavorMaximum(maxVertexInputBindings);
    FavorMaximum(maxVertexInputAttributeOffset);
    FavorMaximum(maxVertexInputBindingStride);
    FavorMaximum(maxVertexOutputComponents);
    FavorMaximum(maxTessellationGenerationLevel);
    FavorMaximum(maxTessellationPatchSize);
    FavorMaximum(maxTessellationControlPerVertexInputComponents);
    FavorMaximum(maxTessellationControlPerVertexOutputComponents);
    FavorMaximum(maxTessellationControlPerPatchOutputComponents);
    FavorMaximum(maxTessellationControlTotalOutputComponents);
    FavorMaximum(maxTessellationEvaluationInputComponents);
    FavorMaximum(maxTessellationEvaluationOutputComponents);
    FavorMaximum(maxGeometryShaderInvocations);
    FavorMaximum(maxGeometryInputComponents);
    FavorMaximum(maxGeometryOutputComponents);
    FavorMaximum(maxGeometryOutputVertices);
    FavorMaximum(maxGeometryTotalOutputComponents);
    FavorMaximum(maxFragmentInputComponents);
    FavorMaximum(maxFragmentOutputAttachments);
    FavorMaximum(maxFragmentDualSrcAttachments);
    FavorMaximum(maxFragmentCombinedOutputResources);
    FavorMaximum(maxComputeSharedMemorySize);
    FavorMaximum(maxComputeWorkGroupCount[0]);
    FavorMaximum(maxComputeWorkGroupCount[1]);
    FavorMaximum(maxComputeWorkGroupCount[2]);
    FavorMaximum(maxComputeWorkGroupInvocations);
    FavorMaximum(maxComputeWorkGroupSize[0]);
    FavorMaximum(maxComputeWorkGroupSize[1]);
    FavorMaximum(maxComputeWorkGroupSize[2]);
    FavorMaximum(subPixelPrecisionBits);
    FavorMaximum(subTexelPrecisionBits);
    FavorMaximum(mipmapPrecisionBits);
    FavorMaximum(maxDrawIndexedIndexValue);
    FavorMaximum(maxDrawIndirectCount);
    FavorMaximum(maxSamplerLodBias);
    FavorMaximum(maxSamplerAnisotropy);
    FavorMaximum(maxViewports);
    FavorMaximum(maxViewportDimensions[0]);
    FavorMaximum(maxViewportDimensions[1]);
    FavorMaximum(viewportBoundsRange[0]);
    FavorMaximum(viewportBoundsRange[1]);
    FavorMaximum(viewportSubPixelBits);
    FavorMinimum(minMemoryMapAlignment);
    FavorMinimum(minTexelBufferOffsetAlignment);
    FavorMinimum(minUniformBufferOffsetAlignment);
    FavorMinimum(minStorageBufferOffsetAlignment);
    FavorMinimum(minTexelOffset);
    FavorMaximum(maxTexelOffset);
    FavorMinimum(minTexelGatherOffset);
    FavorMaximum(maxTexelGatherOffset);
    FavorMinimum(minInterpolationOffset);
    FavorMaximum(maxInterpolationOffset);
    FavorMaximum(subPixelInterpolationOffsetBits);
    FavorMaximum(maxFramebufferWidth);
    FavorMaximum(maxFramebufferHeight);
    FavorMaximum(maxFramebufferLayers);
    FavorMaximum(maxColorAttachments);
    FavorMaximum(maxSampleMaskWords);
    FavorMaximum(maxClipDistances);
    FavorMaximum(maxCullDistances);
    FavorMaximum(maxCombinedClipAndCullDistances);
    FavorMaximum(discreteQueuePriorities);
    FavorMinimum(pointSizeRange[0]);
    FavorMaximum(pointSizeRange[1]);
    FavorMinimum(lineWidthRange[0]);
    FavorMaximum(lineWidthRange[1]);
    FavorMinimum(pointSizeGranularity);
    FavorMinimum(lineWidthGranularity);
  }
// Undef helpers.
#undef FavorMinimum
#undef FavorMaximum

  return score0 >= score1 ? device0 : device1;
}

void Device::create(const Instance &instance, const DeviceCreateInfo &createInfo, const VkAllocationCallbacks *allocator) {
  destroy();
  this->allocator = allocator;
  auto availablePhysicalDevices =
    enumerate<VkPhysicalDevice>([&](auto... args) { return vkEnumeratePhysicalDevices(instance, args...); });
  // If requested, look up device by name.
  if (not createInfo.requestedDeviceName.empty()) {
    for (auto available : availablePhysicalDevices) {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(available, &props);
      if (createInfo.requestedDeviceName == props.deviceName) {
        physicalDevice = available;
        break;
      }
    }
    if (physicalDevice == VK_NULL_HANDLE)
      throw Error(
        std::runtime_error("Can't find Vulkan physical device requested by name '{}'!"_format(createInfo.requestedDeviceName)));
  }
  // If still NULL, default to device-comparison selection routine.
  if (physicalDevice == VK_NULL_HANDLE)
    for (auto available : availablePhysicalDevices)
      if (is_usable(available, createInfo.surface)) physicalDevice = select_better(physicalDevice, available);
  // If still NULL, then no usable device exists!
  if (physicalDevice == VK_NULL_HANDLE) {
    std::string message = "Can't find suitable Vulkan physical device! Checked the "
                          "following physical devices:";
    for (auto available : availablePhysicalDevices) {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(available, &props);
      message += "  {}\n"_format(props.deviceName);
    }
    throw Error(std::runtime_error(message));
  }
  // Retrieve physical device information.
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  queueFamilyProperties = enumerate<VkQueueFamilyProperties>([&](auto... args) {
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, args...);
    return VK_SUCCESS;
  });
  // Find queue family indexes.
  constexpr uint32_t BadIndex = UINT32_MAX;
  queueFamilyIndexes.graphics = BadIndex;
  queueFamilyIndexes.transfer = BadIndex;
  queueFamilyIndexes.compute = BadIndex;
  for (const auto &familyProps : queueFamilyProperties) {
    uint32_t familyIndex = &familyProps - &queueFamilyProperties[0];
    if ((familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) and queueFamilyIndexes.graphics == BadIndex) {
      VkBool32 supported = VK_FALSE;
      VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, createInfo.surface, &supported);
      if (result == VK_SUCCESS and supported == VK_TRUE) queueFamilyIndexes.graphics = familyIndex;
    }
    if ((familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT) and queueFamilyIndexes.transfer == BadIndex)
      queueFamilyIndexes.transfer = familyIndex;
    if ((familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) and queueFamilyIndexes.compute == BadIndex)
      queueFamilyIndexes.compute = familyIndex;
  }
  if (
    queueFamilyIndexes.graphics == BadIndex or queueFamilyIndexes.transfer == BadIndex or
    queueFamilyIndexes.compute == BadIndex)
    throw Error(std::runtime_error("Can't retrieve graphics, transfer, and compute family indexes!"));
  // Finally, create the device.
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  for (uint32_t index : {queueFamilyIndexes.graphics, queueFamilyIndexes.transfer, queueFamilyIndexes.compute})
    queueCreateInfos.emplace_back() = VkDeviceQueueCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueFamilyIndex = index,
      .queueCount = queueFamilyProperties[index].queueCount,
      .pQueuePriorities = nullptr};
  std::vector<const char *> enabledExtensionNames{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueCreateInfoCount = uint32_t(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = nullptr,
    .enabledExtensionCount = uint32_t(enabledExtensionNames.size()),
    .ppEnabledExtensionNames = enabledExtensionNames.data(),
    .pEnabledFeatures = nullptr};
  error_check(vkCreateDevice(physicalDevice, &deviceCreateInfo, allocator, &device));
  // Get default queues.
  vkGetDeviceQueue(device, queueFamilyIndexes.graphics, 0, &defaultQueues.graphics);
  vkGetDeviceQueue(device, queueFamilyIndexes.transfer, 0, &defaultQueues.transfer);
  vkGetDeviceQueue(device, queueFamilyIndexes.compute, 0, &defaultQueues.compute);
  // Create default command pools, for convenience.
  std::pair<uint32_t, VkCommandPool &> commandPools[3] = {
    {queueFamilyIndexes.graphics, defaultCommandPools.graphics},
    {queueFamilyIndexes.transfer, defaultCommandPools.transfer},
    {queueFamilyIndexes.compute, defaultCommandPools.compute}};
  for (auto &[index, commandPool] : commandPools) {
    VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .pNext = NULL, .flags = {}, .queueFamilyIndex = index};
    error_check(vkCreateCommandPool(device, &commandPoolCreateInfo, allocator, &commandPool));
  }
  // Create default sampler, for convenience.
  VkSamplerCreateInfo samplerCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .mipLodBias = 0,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 1,
    .compareEnable = VK_FALSE,
    .compareOp = {},
    .minLod = -1000,
    .maxLod = +1000,
    .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    .unnormalizedCoordinates = VK_FALSE};
  error_check(vkCreateSampler(device, &samplerCreateInfo, allocator, &defaultSampler));
}

void Device::destroy() {
  if (device) {
    vkDestroySampler(device, defaultSampler, allocator);
    vkDestroyCommandPool(device, defaultCommandPools.compute, allocator);
    vkDestroyCommandPool(device, defaultCommandPools.transfer, allocator);
    vkDestroyCommandPool(device, defaultCommandPools.graphics, allocator);
    vkDestroyDevice(device, allocator);
  }
  device = VK_NULL_HANDLE, allocator = nullptr;
  physicalDevice = VK_NULL_HANDLE;
  queueFamilyProperties = {};
  queueFamilyIndexes = {};
  defaultQueues = {};
  defaultCommandPools = {};
  defaultSampler = VK_NULL_HANDLE;
}

VkFormat Device::supportedFormat(
  VkImageTiling tiling, VkFormatFeatureFlags features, std::initializer_list<VkFormat> candidates) const noexcept {
  VkFormatFeatureFlags VkFormatProperties::*relevantFeatures =
    tiling == VK_IMAGE_TILING_OPTIMAL ? &VkFormatProperties::optimalTilingFeatures : &VkFormatProperties::linearTilingFeatures;
  for (auto candidate : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, candidate, &props);
    if ((features & props.*relevantFeatures) == features) return candidate;
  }
  return VK_FORMAT_UNDEFINED;
}

uint32_t Device::findMemoryTypeIndex(VkMemoryPropertyFlags flags, uint32_t type_bits) const {
  VkPhysicalDeviceMemoryProperties props = memoryProperties;
  for (uint32_t index = 0; index < props.memoryTypeCount; index++)
    if ((type_bits & (1 << index)) and (props.memoryTypes[index].propertyFlags & flags) == flags) return index;

  throw Error(std::runtime_error("Can't find a suitable memory type index!"));
  return uint32_t(-1);
}

void SharedDeviceMemory::allocate(
  const Device &device, const std::vector<DeviceMemoryRequest> &requests, const VkAllocationCallbacks *allocator) {
  deallocate();
  this->device = device, this->allocator = allocator;

  struct MemoryTypeInfo {
    /// Allocate info.
    VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = nullptr, .allocationSize = 0, .memoryTypeIndex = 0};
    /// Supported requirement count.
    uint32_t supportedRequirementCount = 0;
    /// Output index in the shared memories array.
    std::optional<uint32_t> sharedIndex = std::nullopt;
  };

  // Gather memory type information.
  const auto &memoryTypeCount = device.memoryProperties.memoryTypeCount;
  const auto &memoryTypes = device.memoryProperties.memoryTypes;
  auto typeSupportsRequest = [&](uint32_t index, const auto &request) {
    const auto &type = memoryTypes[index];
    const auto &[reqs, flags] = request;
    return (reqs.memoryTypeBits & (1 << index)) and (type.propertyFlags & flags) == flags;
  };
  MemoryTypeInfo memoryTypeInfos[VK_MAX_MEMORY_TYPES] = {};
  for (uint32_t index = 0; index < memoryTypeCount; index++) {
    auto &info = memoryTypeInfos[index];
    info.allocateInfo.memoryTypeIndex = index;
    for (const auto &request : requests)
      if (typeSupportsRequest(index, request)) info.supportedRequirementCount++;
  }
  // Find best memory type index for each request.
  auto requestIndexes = vector_reserve<uint32_t>(requests.size());
  for (const auto &request : requests) {
    uint32_t maxCount = 0;
    uint32_t maxCountIndex = UINT32_MAX;
    for (uint32_t index = 0; index < memoryTypeCount; index++) {
      if (typeSupportsRequest(index, request)) {
        auto &info = memoryTypeInfos[index];
        if (maxCount < info.supportedRequirementCount) {
          maxCount = info.supportedRequirementCount;
          maxCountIndex = index;
        }
      }
    }
    if (maxCountIndex == UINT32_MAX) throw Error(std::runtime_error("Can't find suitable memory type!"));
    requestIndexes.push_back(maxCountIndex);
  }
  // Determine memory views.
  memories.reserve(memoryTypeCount);
  memoryViews.reserve(requests.size());
  for (auto &&[request, requestIndex] : ranges::zip(requests, requestIndexes)) {
    auto &memoryTypeInfo = memoryTypeInfos[requestIndex];
    if (memoryTypeInfo.sharedIndex == std::nullopt) {
      memoryTypeInfo.sharedIndex = memories.size();
      memories.emplace_back();
    }
    // If necessary, round allocation size up to required alignment.
    auto &allocSize = memoryTypeInfo.allocateInfo.allocationSize;
    if (allocSize % request.requirements.alignment)
      allocSize = allocSize - allocSize % request.requirements.alignment + request.requirements.alignment;
    memoryViews.emplace_back() = {.memory = VK_NULL_HANDLE, .offset = allocSize, .size = request.requirements.size};
    /// Increment allocation size.
    allocSize += request.requirements.size;
  }
  // Allocate memory.
  for (auto &memoryTypeInfo : memoryTypeInfos)
    if (memoryTypeInfo.sharedIndex)
      error_check(vkAllocateMemory(device, &memoryTypeInfo.allocateInfo, allocator, &memories[*memoryTypeInfo.sharedIndex]));

  // Finally, link views to the associated memory handles.
  for (auto &&[memoryView, requestIndex] : ranges::zip(memoryViews, requestIndexes))
    memoryView.memory = memories[*memoryTypeInfos[requestIndex].sharedIndex];
}

void SharedDeviceMemory::deallocate() {
  if (device)
    for (auto memory : memories)
      if (memory) vkFreeMemory(device, memory, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  memories.clear();
  memoryViews.clear();
}

} // namespace mi::vk
