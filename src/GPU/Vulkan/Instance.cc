#include "Microcosm/GPU/Vulkan/Instance"

namespace mi::vk {

void Instance::create(const InstanceCreateInfo &createInfo, const VkAllocationCallbacks *allocator) {
  destroy();
  this->allocator = allocator;
  // Initialize enabled instance layers.
  enabledLayerNames.reserve(createInfo.requestedLayerNames.size());
  auto layerProps = enumerate<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
  for (auto &name : createInfo.requestedLayerNames)
    for (auto &props : layerProps)
      if (std::string_view(name) == props.layerName) enabledLayerNames.push_back(name);
  // Initialize enabled instance extensions.
  enabledExtensionNames.reserve(createInfo.requestedExtensionNames.size());
  auto extensionProps =
    enumerate<VkExtensionProperties>([](auto... args) { return vkEnumerateInstanceExtensionProperties(nullptr, args...); });
  for (auto &name : createInfo.requestedExtensionNames)
    for (auto &props : extensionProps)
      if (std::string_view(name) == props.extensionName) enabledExtensionNames.push_back(name);
  // Now create the instance.
  VkApplicationInfo applicationInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = createInfo.applicationName,
    .applicationVersion = createInfo.applicationVersion,
    .pEngineName = "microcosm",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = createInfo.apiVersion};
  VkInstanceCreateInfo instanceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .pApplicationInfo = &applicationInfo,
    .enabledLayerCount = uint32_t(enabledLayerNames.size()),
    .ppEnabledLayerNames = enabledLayerNames.data(),
    .enabledExtensionCount = uint32_t(enabledExtensionNames.size()),
    .ppEnabledExtensionNames = enabledExtensionNames.data()};
  error_check(vkCreateInstance(&instanceCreateInfo, allocator, &instance));
}

void Instance::destroy() {
  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, allocator);
  }
  instance = VK_NULL_HANDLE;
  allocator = nullptr;
}

} // namespace mi::vk
