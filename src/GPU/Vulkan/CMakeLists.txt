find_package(Vulkan QUIET)
if (Vulkan_FOUND)
  message(STATUS "Found Vulkan, enabling ${PROJECT_NAME}::GPU_Vulkan.")
  microcosm_add_library(
    "GPU_Vulkan"
    SHARED
    SOURCES
      "Buffer.cc"
      "CommandBuffer.cc"
      "common.cc"
      "Device.cc"
      "Fence.cc"
      "Pipeline.cc"
      "Image.cc"
      "Instance.cc"
      "Semaphore.cc"
      "Shader.cc"
    DEPENDS Vulkan::Vulkan
    EXPORT_MACRO "MI_VK_API"
    EXPORT_FILENAME "Microcosm/GPU/Vulkan/Export.h"
    )
endif()
