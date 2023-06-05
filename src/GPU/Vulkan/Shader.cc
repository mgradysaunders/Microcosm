#include "Microcosm/GPU/Vulkan/Shader"

#include <cerrno>
#include <fstream>

namespace mi::vk {

static void readCodeFile(std::string_view filename, std::vector<uint32_t> &code) {
  std::ifstream stream(std::string(filename), std::ios::in | std::ios::binary);
  if (!stream.is_open()) throw std::runtime_error("Can't read {}: {}"_format(filename, std::strerror(errno)));

  stream.seekg(0, std::ios::end);
  size_t size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  if (size % 4) throw std::runtime_error("Can't read {}: size not a multiple of 4"_format(filename));
  code.resize(size / 4);
  stream.read(reinterpret_cast<char *>(code.data()), size);
}

void Shaders::create(
  VkDevice device, const std::vector<ShaderCreateInfo> &createInfos, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;
  std::vector<uint32_t> code;
  for (const auto &createInfo : createInfos) {
    // Create shader.
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = uint32_t(createInfo.code.size()) * 4,
      .pCode = createInfo.code.data()};
    if (shaderModuleCreateInfo.codeSize == 0) { // Load from file?
      readCodeFile(createInfo.codeFilename, code);
      shaderModuleCreateInfo.codeSize = uint32_t(code.size()) * 4;
      shaderModuleCreateInfo.pCode = code.data();
    }
    error_check(vkCreateShaderModule(device, &shaderModuleCreateInfo, allocator, &shaders.emplace_back()));

    // Set up stage create info.
    stageCreateInfos.emplace_back() = VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = createInfo.stage,
      .module = shaders.back(),
      .pName = "main",
      .pSpecializationInfo = nullptr};
  }
}

void Shaders::destroy() {
  if (device)
    for (auto shader : shaders) vkDestroyShaderModule(device, shader, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  shaders.clear();
  stageCreateInfos.clear();
}

} // namespace mi::vk
