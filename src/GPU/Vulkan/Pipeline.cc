#include "Microcosm/GPU/Vulkan/Pipeline"

namespace mi::vk {

static VkGraphicsPipelineCreateInfo convertToVkCreateInfo(const GraphicsPipelineCreateInfo &createInfo) {
  auto derefOrDefault = []<typename Value>(const Value *value) { return value ? *value : Value(); };
  const auto &inputState = *createInfo.inputState;
  const auto &vertexBindings = inputState.vertexBindings;
  const auto &vertexAttributes = inputState.vertexAttributes;
  const auto depthState = derefOrDefault(createInfo.depthState);
  const auto depthBounds = depthState.bounds.value_or(DepthBounds());
  const auto depthBias = depthState.bias.value_or(DepthBias());
  const auto multisampleState = derefOrDefault(createInfo.multisampleState);
  const auto stencilState = derefOrDefault(createInfo.stencilState);
  return VkGraphicsPipelineCreateInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = uint32_t(createInfo.stages.size()),
    .pStages = createInfo.stages.data(),
    .pVertexInputState =
      new VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .vertexBindingDescriptionCount = uint32_t(vertexBindings.size()),
        .pVertexBindingDescriptions = vertexBindings.data(),
        .vertexAttributeDescriptionCount = uint32_t(vertexAttributes.size()),
        .pVertexAttributeDescriptions = vertexAttributes.data()},
    .pInputAssemblyState =
      new VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .topology = inputState.topology,
        .primitiveRestartEnable = inputState.primitiveRestartEnable},
    .pTessellationState = inputState.patchControlPoints > 0
                            ? (new VkPipelineTessellationStateCreateInfo{
                                .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
                                .pNext = nullptr,
                                .flags = {},
                                .patchControlPoints = inputState.patchControlPoints})
                            : nullptr,
    .pViewportState = !createInfo.viewports.empty() ? (new VkPipelineViewportStateCreateInfo{
                                                        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                        .pNext = nullptr,
                                                        .flags = 0,
                                                        .viewportCount = uint32_t(createInfo.viewports.size()),
                                                        .pViewports = createInfo.viewports.data(),
                                                        .scissorCount = uint32_t(createInfo.scissors.size()),
                                                        .pScissors = createInfo.scissors.data()})
                                                    : nullptr,
    .pRasterizationState =
      new VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .depthClampEnable = depthState.clampEnable,
        .rasterizerDiscardEnable = inputState.rasterizerDiscardEnable,
        .polygonMode = inputState.polygonMode,
        .cullMode = inputState.cullMode,
        .frontFace = inputState.frontFace,
        .depthBiasEnable = depthState.bias.has_value(),
        .depthBiasConstantFactor = depthBias.constantFactor,
        .depthBiasClamp = depthBias.clamp,
        .depthBiasSlopeFactor = depthBias.slopeFactor,
        .lineWidth = inputState.lineWidth},
    .pMultisampleState = createInfo.multisampleState or !inputState.rasterizerDiscardEnable
                           ? (new VkPipelineMultisampleStateCreateInfo{
                               .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = {},
                               .rasterizationSamples = multisampleState.samples,
                               .sampleShadingEnable = multisampleState.minSampleShading.has_value(),
                               .minSampleShading = multisampleState.minSampleShading.value_or(0.0f),
                               .pSampleMask = multisampleState.sampleMask.data(),
                               .alphaToCoverageEnable = multisampleState.alphaToCoverageEnable,
                               .alphaToOneEnable = multisampleState.alphaToOneEnable})
                           : nullptr,
    .pDepthStencilState = createInfo.depthState || createInfo.stencilState
                            ? (new VkPipelineDepthStencilStateCreateInfo{
                                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                .pNext = nullptr,
                                .flags = {},
                                .depthTestEnable = createInfo.depthState ? VK_TRUE : VK_FALSE,
                                .depthWriteEnable = depthState.writeEnable,
                                .depthCompareOp = depthState.compareOp,
                                .depthBoundsTestEnable = depthState.bounds.has_value(),
                                .stencilTestEnable = createInfo.stencilState ? VK_TRUE : VK_FALSE,
                                .front = stencilState.front,
                                .back = stencilState.back,
                                .minDepthBounds = depthBounds.minBounds,
                                .maxDepthBounds = depthBounds.maxBounds})
                            : nullptr,
    .pColorBlendState =
      new VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .logicOpEnable = createInfo.logicOp.has_value(),
        .logicOp = createInfo.logicOp.value_or(VkLogicOp()),
        .attachmentCount = uint32_t(createInfo.blendAttachments.size()),
        .pAttachments = createInfo.blendAttachments.data(),
        .blendConstants =
          {createInfo.blendConstants[0], createInfo.blendConstants[1], createInfo.blendConstants[2],
           createInfo.blendConstants[3]}},
    .pDynamicState = !createInfo.dynamicStates.empty() ? (new VkPipelineDynamicStateCreateInfo{
                                                           .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                           .pNext = nullptr,
                                                           .flags = 0,
                                                           .dynamicStateCount = uint32_t(createInfo.dynamicStates.size()),
                                                           .pDynamicStates = createInfo.dynamicStates.data()})
                                                       : nullptr,
    .layout = createInfo.layout,
    .renderPass = createInfo.renderPass,
    .subpass = createInfo.subpass,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = createInfo.basePipeline};
}

void GraphicsPipelines::create(
  VkDevice device, const std::vector<GraphicsPipelineCreateInfo> &createInfos, const VkAllocationCallbacks *allocator) {
  destroy();
  this->device = device, this->allocator = allocator;

  std::vector<VkGraphicsPipelineCreateInfo> vkCreateInfos;
  Scope vkCreateInfoScope{
    [&] {
      vkCreateInfos.reserve(createInfos.size());
      for (auto &createInfo : createInfos) vkCreateInfos.emplace_back(convertToVkCreateInfo(createInfo));
    },
    [&] {
      for (auto &vkCreateInfo : vkCreateInfos) {
        delete vkCreateInfo.pVertexInputState;
        delete vkCreateInfo.pInputAssemblyState;
        delete vkCreateInfo.pTessellationState;
        delete vkCreateInfo.pViewportState;
        delete vkCreateInfo.pRasterizationState;
        delete vkCreateInfo.pMultisampleState;
        delete vkCreateInfo.pDepthStencilState;
        delete vkCreateInfo.pColorBlendState;
        delete vkCreateInfo.pDynamicState;
      }
      vkCreateInfos.clear();
    }};

  pipelines.resize(createInfos.size());
  VkResult result =
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, vkCreateInfos.size(), vkCreateInfos.data(), allocator, pipelines.data());
  error_check(result);
}

void GraphicsPipelines::destroy() {
  if (device)
    for (auto &pipeline : pipelines) vkDestroyPipeline(device, pipeline, allocator);
  device = VK_NULL_HANDLE, allocator = nullptr;
  pipelines.clear();
}

} // namespace mi::vk
