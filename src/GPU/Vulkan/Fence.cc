#include "Microcosm/GPU/Vulkan/Fence"

namespace mi::vk {

void Fence::create(VkDevice device, VkFenceCreateFlags flags, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  VkFenceCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = flags};
  error_check(vkCreateFence(device, &create_info, allocator, &fence));
}

void Fence::destroy() {
  if (device) vkDestroyFence(device, fence, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  fence = VK_NULL_HANDLE;
}

void Fences::create(VkDevice device, uint32_t count, VkFenceCreateFlags flags, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  VkFenceCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = flags};
  fences.resize(count);
  for (auto &fence : fences) error_check(vkCreateFence(device, &create_info, allocator, &fence));
}

void Fences::destroy() {
  if (device)
    for (auto &fence : fences) vkDestroyFence(device, fence, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  fences.clear();
}

} // namespace mi::vk
