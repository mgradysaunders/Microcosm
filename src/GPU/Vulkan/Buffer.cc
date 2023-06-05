#include "Microcosm/GPU/Vulkan/Buffer"
#include "Microcosm/GPU/Vulkan/CommandBuffer"

namespace mi::vk {

void Buffer::create(const Device &device, const BufferCreateInfo &createInfo, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  error_check(vkCreateBuffer(device, &createInfo.info, allocator, &buffer));
  VkMemoryRequirements reqs;
  vkGetBufferMemoryRequirements(device, buffer, &reqs);
  VkMemoryAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = reqs.size,
    .memoryTypeIndex = device.findMemoryTypeIndex(createInfo.propertyFlags, reqs.memoryTypeBits)};
  error_check(vkAllocateMemory(device, &allocate_info, allocator, &memory));
  error_check(vkBindBufferMemory(device, buffer, memory, 0));
  memorySize = reqs.size;
}

void Buffer::destroy() {
  if (device) {
    if (buffer) vkDestroyBuffer(device, buffer, allocator);
    if (memory) vkFreeMemory(device, memory, allocator);
  }
  device = VK_NULL_HANDLE, allocator = nullptr;
  buffer = VK_NULL_HANDLE;
  memory = VK_NULL_HANDLE;
  memorySize = 0;
}

void Buffers::create(
  const Device &device, const std::vector<BufferCreateInfo> &createInfos, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  // Create buffers.
  buffers.reserve(createInfos.size());
  auto requests = vector_reserve<DeviceMemoryRequest>(createInfos.size());
  for (auto &createInfo : createInfos) {
    auto &buffer = buffers.emplace_back();
    auto &request = requests.emplace_back();
    error_check(vkCreateBuffer(device, &createInfo.info, allocator, &buffer));
    vkGetBufferMemoryRequirements(device, buffer, &request.requirements);
    request.flags = createInfo.propertyFlags;
  }
  // Allocate memory and bind to buffers.
  memory.allocate(device, requests, allocator);
  for (auto &&[buffer, memoryView] : ranges::zip(buffers, memory))
    error_check(vkBindBufferMemory(device, buffer, memoryView.memory, memoryView.offset));
}

void Buffers::destroy() {
  if (device)
    for (auto buffer : buffers) vkDestroyBuffer(device, buffer, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  buffers.clear();
  memory.deallocate();
}

void bufferGetData(const Device &device, const BufferAccess &access, void *mem) {
  Buffer staging(device, BufferCreateInfo::stagingDst(access.size));
  bufferCopy(device, access.buffer, staging, {VkBufferCopy{access.offset, 0, access.size}});
  map_memory(device, staging.memory, 0, access.size, [&](void *ptr) { std::memcpy(mem, ptr, access.size); });
}

void bufferSetData(const Device &device, const BufferAccess &access, const void *mem) {
  Buffer staging(device, BufferCreateInfo::stagingSrc(access.size));
  map_memory(device, staging.memory, 0, access.size, [&](void *ptr) { std::memcpy(ptr, mem, access.size); });
  bufferCopy(device, staging, access.buffer, {VkBufferCopy{0, access.offset, access.size}});
}

void bufferCopy(const Device &device, VkBuffer srcBuffer, VkBuffer dstBuffer, IteratorRange<const VkBufferCopy *> regions) {
  if (srcBuffer == dstBuffer or regions.empty()) return;
  CommandBuffer cmd(device, device.defaultCommandPools.transfer);
  cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, regions.size(), regions.data());
  cmd.end();
  cmd.flush(device.defaultQueues.transfer);
}

} // namespace mi::vk
