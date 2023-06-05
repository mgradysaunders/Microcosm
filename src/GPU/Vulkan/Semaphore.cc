#include "Microcosm/GPU/Vulkan/Semaphore"

namespace mi::vk {

void Semaphore::create(VkDevice device, VkSemaphoreCreateFlags flags, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  VkSemaphoreCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = flags};
  error_check(vkCreateSemaphore(device, &create_info, allocator, &semaphore));
}

void Semaphore::destroy() {
  if (device) vkDestroySemaphore(device, semaphore, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  semaphore = VK_NULL_HANDLE;
}

void Semaphores::create(VkDevice device, uint32_t count, VkSemaphoreCreateFlags flags, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  VkSemaphoreCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = flags};
  semaphores.resize(count);
  for (auto &semaphore : semaphores) error_check(vkCreateSemaphore(device, &create_info, allocator, &semaphore));
}

void Semaphores::destroy() {
  if (device)
    for (auto &semaphore : semaphores) vkDestroySemaphore(device, semaphore, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  semaphores.clear();
}

} // namespace mi::vk
