#include "Microcosm/GPU/Vulkan/Image"
#include "Microcosm/GPU/Vulkan/Buffer"
#include "Microcosm/GPU/Vulkan/CommandBuffer"

namespace mi::vk {

void Image::create(const Device &device, const ImageCreateInfo &createInfo, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  error_check(vkCreateImage(device, &createInfo.info, allocator, &image));
  VkMemoryRequirements reqs;
  vkGetImageMemoryRequirements(device, image, &reqs);
  VkMemoryAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = reqs.size,
    .memoryTypeIndex = device.findMemoryTypeIndex(createInfo.propertyFlags, reqs.memoryTypeBits)};
  error_check(vkAllocateMemory(device, &allocInfo, allocator, &memory));
  error_check(vkBindImageMemory(device, image, memory, 0));
  imageView = imageCreateDefaultView(device, image, createInfo.info, allocator);
}

void Image::destroy() {
  if (device) {
    vkDestroyImageView(device, imageView, allocator);
    vkDestroyImage(device, image, allocator);
    vkFreeMemory(device, memory, allocator);
  }
  device = VK_NULL_HANDLE, allocator = nullptr;
  image = VK_NULL_HANDLE;
  imageView = VK_NULL_HANDLE;
  memory = VK_NULL_HANDLE;
}

void Images::create(
  const Device &device, const std::vector<ImageCreateInfo> &createInfos, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  // Create images.
  images.reserve(createInfos.size());
  auto requests = vector_reserve<DeviceMemoryRequest>(createInfos.size());
  for (auto &createInfo : createInfos) {
    auto &image = images.emplace_back();
    auto &request = requests.emplace_back();
    error_check(vkCreateImage(device, &createInfo.info, allocator, &image));
    vkGetImageMemoryRequirements(device, image, &request.requirements);
    request.flags = createInfo.propertyFlags;
  }
  // Allocate memory and bind to images.
  memory.allocate(device, requests, allocator);
  for (auto &&[image, memoryView] : ranges::zip(images, memory))
    error_check(vkBindImageMemory(device, image, memoryView.memory, memoryView.offset));
  /// Create default image views.
  imageViews.reserve(createInfos.size());
  for (auto &&[image, createInfo] : ranges::zip(images, createInfos))
    imageViews.emplace_back() = imageCreateDefaultView(device, image, createInfo.info, allocator);
}

void Images::destroy() {
  if (device) {
    for (auto imageView : imageViews) vkDestroyImageView(device, imageView, allocator);
    for (auto image : images) vkDestroyImage(device, image, allocator);
  }
  device = VK_NULL_HANDLE, allocator = nullptr;
  images.clear();
  imageViews.clear();
  memory.deallocate();
}

VkImageView imageCreateDefaultView(
  VkDevice device, VkImage image, const VkImageCreateInfo &createInfo, const VkAllocationCallbacks *allocator) {
  static_assert(
    int(VK_IMAGE_TYPE_1D) == int(VK_IMAGE_VIEW_TYPE_1D) and int(VK_IMAGE_TYPE_2D) == int(VK_IMAGE_VIEW_TYPE_2D) and
    int(VK_IMAGE_TYPE_3D) == int(VK_IMAGE_VIEW_TYPE_3D));
  VkImageViewType viewType = VkImageViewType(createInfo.imageType);
  // Is cube compatible with multiple of 6 array layers?
  if ((createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) and createInfo.arrayLayers % 6 == 0)
    viewType = createInfo.arrayLayers > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
  // Is 1D image with more than 1 array layer?
  else if (createInfo.imageType == VK_IMAGE_TYPE_1D and createInfo.arrayLayers > 1)
    viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
  // Is 2D image with more than 1 array layer?
  else if (createInfo.imageType == VK_IMAGE_TYPE_2D and createInfo.arrayLayers > 1)
    viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // Is depth format?
  if (
    createInfo.format == VK_FORMAT_D16_UNORM or createInfo.format == VK_FORMAT_X8_D24_UNORM_PACK32 or
    createInfo.format == VK_FORMAT_D32_SFLOAT)
    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Is stencil format?
  else if (createInfo.format == VK_FORMAT_S8_UINT)
    aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
  // Is depth/stencil format?
  else if (
    createInfo.format == VK_FORMAT_D16_UNORM_S8_UINT || createInfo.format == VK_FORMAT_D24_UNORM_S8_UINT ||
    createInfo.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  VkImageView imageView = VK_NULL_HANDLE;
  VkImageViewCreateInfo imageViewCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .flags = {},
    .image = image,
    .viewType = viewType,
    .format = createInfo.format,
    .components =
      {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
    .subresourceRange = {
      .aspectMask = aspectMask,
      .baseMipLevel = 0,
      .levelCount = createInfo.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = createInfo.arrayLayers}};
  error_check(vkCreateImageView(device, &imageViewCreateInfo, allocator, &imageView));
  return imageView;
}

void imageCmdTransitionLayout(
  VkCommandBuffer commandBuffer,
  VkImage image,
  VkImageLayout oldLayout,
  VkImageLayout newLayout,
  VkImageSubresourceRange subresourceRange,
  VkPipelineStageFlags srcStageMask,
  VkPipelineStageFlags dstStageMask) {
  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = nullptr,
    .srcAccessMask = 0, // Uninitialized
    .dstAccessMask = 0, // Uninitialized
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = subresourceRange};
  // Initialize srcAccessMask.
  switch (oldLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED: break;
  case VK_IMAGE_LAYOUT_PREINITIALIZED: barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT; break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT; break;
  default: throw Error(std::logic_error("Unhandled layout!")); break;
  }
  // Initialize dstAccessMask.
  switch (newLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    if (barrier.srcAccessMask == VkAccessFlags(0))
      barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default: throw Error(std::logic_error("Unhandled layout!")); break;
  }
  // Record barrier.
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void imageTransitionLayout(
  const Device &device,
  VkImage image,
  VkImageLayout oldLayout,
  VkImageLayout newLayout,
  VkImageSubresourceRange subresourceRange) {
  CommandBuffer cmd(device, device.defaultCommandPools.graphics);
  cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  imageCmdTransitionLayout(cmd, image, oldLayout, newLayout, subresourceRange);
  cmd.end();
  cmd.flush(device.defaultQueues.graphics);
}

void imageGetBuffer(
  const Device &device,
  VkImage image,
  VkImageLayout imageLayout,
  VkBuffer buffer,
  IteratorRange<const VkBufferImageCopy *> regions) {
  assert(
    imageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or imageLayout == VK_IMAGE_LAYOUT_GENERAL or
    imageLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR);
  CommandBuffer cmd(device, device.defaultCommandPools.transfer);
  cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  vkCmdCopyImageToBuffer(cmd, image, imageLayout, buffer, regions.size(), regions.data());
  cmd.end();
  cmd.flush(device.defaultQueues.transfer);
}

void imageSetBuffer(
  const Device &device,
  VkImage image,
  VkImageLayout imageLayout,
  VkBuffer buffer,
  IteratorRange<const VkBufferImageCopy *> regions) {
  assert(
    imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or imageLayout == VK_IMAGE_LAYOUT_GENERAL or
    imageLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR);
  CommandBuffer cmd(device, device.defaultCommandPools.transfer);
  cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  vkCmdCopyBufferToImage(cmd, buffer, image, imageLayout, regions.size(), regions.data());
  cmd.end();
  cmd.flush(device.defaultQueues.transfer);
}

void imageGetData(const Device &device, const ImageAccess &access, void *mem) {
  Buffer staging(device, BufferCreateInfo::stagingDst(access.size));
  imageGetBuffer(
    device, access.image, access.imageLayout, staging,
    {VkBufferImageCopy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = access.subresourceLayers,
      .imageOffset = access.offset,
      .imageExtent = access.extent}});
  map_memory(device, staging.memory, 0, access.size, [&](void *ptr) { std::memcpy(mem, ptr, access.size); });
}

void imageSetData(const Device &device, const ImageAccess &access, const void *mem) {
  Buffer staging(device, BufferCreateInfo::stagingSrc(access.size));
  map_memory(device, staging.memory, 0, access.size, [&](void *ptr) { std::memcpy(ptr, mem, access.size); });
  imageSetBuffer(
    device, access.image, access.imageLayout, staging,
    {VkBufferImageCopy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = access.subresourceLayers,
      .imageOffset = access.offset,
      .imageExtent = access.extent}});
}

} // namespace mi::vk
