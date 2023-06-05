#include "Microcosm/GPU/Vulkan/CommandBuffer"
#include "Microcosm/GPU/Vulkan/Fence"

namespace mi::vk {

void CommandBuffer::allocate(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level) {
  deallocate();
  this->device = device;
  commandPool = pool;
  VkCommandBufferAllocateInfo allocateInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,
    .commandPool = pool,
    .level = level,
    .commandBufferCount = 1};
  error_check(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));
}

void CommandBuffer::deallocate() {
  if (device) vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  device = VK_NULL_HANDLE;
  commandPool = VK_NULL_HANDLE;
  commandBuffer = VK_NULL_HANDLE;
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags, const VkCommandBufferInheritanceInfo *inheritanceInfo) {
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = flags,
    .pInheritanceInfo = inheritanceInfo};
  error_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

void CommandBuffer::flush(VkQueue queue) {
  Fence fence{device};
  VkSubmitInfo submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .pWaitDstStageMask = nullptr,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = nullptr};
  error_check(vkQueueSubmit(queue, 1, &submitInfo, fence.fence));
  void(fence.wait());
}

} // namespace mi::vk
