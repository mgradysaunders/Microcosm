#include "Microcosm/GPU/Vulkan/common"

#include "vulkan/vk_enum_string_helper.h"

namespace mi::vk {

std::string_view to_string(VkResult result) noexcept { return string_VkResult(result); }

VkResult error_check(VkResult result, const std::source_location &location) {
  if (result < 0) throw Error(std::runtime_error(string_VkResult(result)), location);
  return result;
}

} // namespace mi::vk
