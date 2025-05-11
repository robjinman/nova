#include "vulkan/pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"
#include "vulkan/render_resources.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include "model.hpp"
#include "logger.hpp"
#include <shaderc/shaderc.hpp>
#include <array>
#include <numeric>
#include <cstring>

namespace
{

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& code)
{
  VkShaderModuleCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = code.size() * sizeof(uint32_t),
    .pCode = code.data()
  };

  VkShaderModule shaderModule;
  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
    "Failed to create shader module");

  return shaderModule;
}

VkFormat attributeFormat(BufferUsage usage)
{
  switch (usage) {
    case BufferUsage::AttrPosition: return VK_FORMAT_R32G32B32_SFLOAT;
    case BufferUsage::AttrNormal: return VK_FORMAT_R32G32B32_SFLOAT;
    case BufferUsage::AttrTexCoord: return VK_FORMAT_R32G32_SFLOAT;
    case BufferUsage::AttrTangent: return VK_FORMAT_R32G32B32_SFLOAT;
    case BufferUsage::AttrJointIndices: return VK_FORMAT_R8G8B8A8_UINT;
    case BufferUsage::AttrJointWeights: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default: EXCEPTION("Buffer type is not a vertex attribute");
  }
}

std::vector<VkVertexInputAttributeDescription>
  createAttributeDescriptions(const VertexLayout& layout)
{
  std::vector<VkVertexInputAttributeDescription> attributes;

  const uint32_t first = static_cast<uint32_t>(BufferUsage::AttrPosition);
  for (auto& attribute : layout) {
    if (attribute == BufferUsage::None) {
      break;
    }

    attributes.push_back(VkVertexInputAttributeDescription{
      .location = static_cast<uint32_t>(attribute) - first,
      .binding = 0,
      .format = attributeFormat(attribute),
      .offset = static_cast<uint32_t>(calcOffsetInVertex(layout, attribute))
    });
  }

  return attributes;
}

VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState()
{
  return VkPipelineInputAssemblyStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };
}

VkPipelineViewportStateCreateInfo defaultViewportState(VkViewport& viewport, VkRect2D& scissor,
  VkExtent2D swapchainExtent)
{
  viewport = VkViewport{
    .x = 0.f,
    .y = 0.f,
    .width = static_cast<float>(swapchainExtent.width),
    .height = static_cast<float>(swapchainExtent.height),
    .minDepth = 0.f,
    .maxDepth = 1.f
  };

  scissor = VkRect2D{
    .offset = { 0, 0 },
    .extent = swapchainExtent
  };

  return VkPipelineViewportStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };
}

VkPipelineRasterizationStateCreateInfo defaultRasterizationState(bool doubleSided)
{
  VkCullModeFlags cullMode = doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
  return VkPipelineRasterizationStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = cullMode,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
  };
}

VkPipelineMultisampleStateCreateInfo defaultMultisamplingState()
{
  return VkPipelineMultisampleStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };
}

VkPipelineColorBlendStateCreateInfo
defaultColourBlendState(VkPipelineColorBlendAttachmentState& colourBlendAttachment)
{
  colourBlendAttachment = VkPipelineColorBlendAttachmentState{
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT
  };

  return VkPipelineColorBlendStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_AND,
    .attachmentCount = 1,
    .pAttachments = &colourBlendAttachment,
    .blendConstants = { 0, 0, 0, 0 }
  };
}

VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState()
{
  return VkPipelineDepthStencilStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {},
    .minDepthBounds = 0.f,
    .maxDepthBounds = 1.f
  };
}

class SourceIncluder : public shaderc::CompileOptions::IncluderInterface
{
  public:
    SourceIncluder(const FileSystem& fileSystem)
      : m_fileSystem(fileSystem) {}

    shaderc_include_result* GetInclude(const char* requested_source,
      shaderc_include_type type, const char* requesting_source, size_t include_depth) override;

    void ReleaseInclude(shaderc_include_result* data) override;

  private:
    const FileSystem& m_fileSystem;
    std::string m_errorMessage;
};

shaderc_include_result* SourceIncluder::GetInclude(const char* requested_source,
  shaderc_include_type, const char*, size_t)
{
  auto result = new shaderc_include_result{};

  try {
    const std::filesystem::path sourcesDir = "shaders";
    auto sourcePath = sourcesDir / requested_source;

    size_t sourceNameLength = sourcePath.string().length();
    char* nameBuffer = new char[sourceNameLength];
    memcpy(nameBuffer, reinterpret_cast<const char*>(sourcePath.c_str()), sourceNameLength);

    result->source_name = nameBuffer;
    result->source_name_length = sourceNameLength;

    auto source = m_fileSystem.readFile(sourcePath);
    size_t contentBufferLength = source.size();
    char* contentBuffer = new char[contentBufferLength];
    memcpy(contentBuffer, source.data(), source.size());

    result->content = contentBuffer;
    result->content_length = contentBufferLength;
    result->user_data = nullptr;
  }
  catch (const std::exception& ex) {
    m_errorMessage = ex.what();
    result->content = m_errorMessage.c_str();
    result->content_length = m_errorMessage.length();
  }

  return result;
}

void SourceIncluder::ReleaseInclude(shaderc_include_result* data)
{
  if (data) {
    if (data->content) {
      delete[] data->content;
    }
    if (data->source_name) {
      delete[] data->source_name;
    }
    delete data;
  }
}

struct ShaderProgram
{
  std::vector<uint32_t> vertexShaderCode;
  std::vector<uint32_t> fragmentShaderCode;
};

enum class ShaderType
{
  Vertex,
  Fragment
};

class PipelineImpl : public Pipeline
{
  public:
    PipelineImpl(RenderPass renderPass, const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures, const FileSystem& fileSystem,
      const RenderResources& renderResources, Logger& logger, VkDevice device,
      VkExtent2D swapchainExtent, VkFormat swapchainImageFormat, VkFormat depthFormat);

    void onViewportResize(VkExtent2D swapchainExtent) override;

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      BindState& bindState, size_t currentFrame) override;

    ~PipelineImpl() override;

  private:
    Logger& m_logger;
    const FileSystem& m_fileSystem;
    const RenderResources& m_renderResources;
    RenderPass m_renderPass;
    VkDevice m_device;
    VkFormat m_swapchainImageFormat;
    VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo m_vertShaderStageInfo;
    VkPipelineShaderStageCreateInfo m_fragShaderStageInfo;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptions;
    std::vector<VkVertexInputBindingDescription> m_vertexBindingDescriptions;
    VkPipelineVertexInputStateCreateInfo m_vertexInputStateInfo;
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    VkPipelineLayoutCreateInfo m_layoutInfo;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyStateInfo;
    VkViewport m_viewport;
    VkRect2D m_scissor;
    VkPipelineRasterizationStateCreateInfo m_rasterizationStateInfo;
    VkPipelineMultisampleStateCreateInfo m_multisampleStateInfo;
    VkPipelineColorBlendAttachmentState m_colourBlendAttachmentState;
    VkPipelineColorBlendStateCreateInfo m_colourBlendStateInfo;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilStateInfo;
    VkPipelineRenderingCreateInfo m_renderingCreateInfo;

    ShaderProgram compileShaderProgram(RenderPass renderPass, const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures);

    std::vector<uint32_t> compileShader(const std::string& name, const std::vector<char>& source,
      ShaderType type, const std::vector<std::string>& defines);

    void constructPipeline(VkExtent2D swapchainExtent);
    void destroyPipeline();
};

PipelineImpl::PipelineImpl(RenderPass renderPass, const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures, const FileSystem& fileSystem,
  const RenderResources& renderResources, Logger& logger, VkDevice device,
  VkExtent2D swapchainExtent, VkFormat swapchainImageFormat, VkFormat depthFormat)
  : m_logger(logger)
  , m_fileSystem(fileSystem)
  , m_renderResources(renderResources)
  , m_renderPass(renderPass)
  , m_device(device)
  , m_swapchainImageFormat(swapchainImageFormat)
{
  auto program = compileShaderProgram(renderPass, meshFeatures, materialFeatures);

  m_vertShaderModule = createShaderModule(m_device, program.vertexShaderCode);
  m_fragShaderModule = createShaderModule(m_device, program.fragmentShaderCode);

  m_vertShaderStageInfo = VkPipelineShaderStageCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = m_vertShaderModule,
    .pName = "main",
    .pSpecializationInfo = nullptr
  };

  m_fragShaderStageInfo = VkPipelineShaderStageCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = m_fragShaderModule,
    .pName = "main",
    .pSpecializationInfo = nullptr
  };

  size_t vertexSize = 0;
  for (auto usage : meshFeatures.vertexLayout) {
    vertexSize += getAttributeSize(usage);
  }

  VkVertexInputBindingDescription vertexBindingDescription{
    .binding = 0,
    .stride = static_cast<uint32_t>(vertexSize),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  m_vertexAttributeDescriptions = createAttributeDescriptions(meshFeatures.vertexLayout);
  if (meshFeatures.flags.test(MeshFeatures::IsInstanced)) {
    for (unsigned int i = 0; i < 4; ++i) {
      uint32_t offset = offsetof(MeshInstance, modelMatrix) + 4 * sizeof(float_t) * i;
  
      VkVertexInputAttributeDescription attr{
        .location = (LAST_ATTR_IDX - static_cast<uint32_t>(BufferUsage::AttrPosition)) + 1 + i,
        .binding = 1,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offset
      };
  
      m_vertexAttributeDescriptions.push_back(attr);
    }
  }

  m_vertexBindingDescriptions = {
    vertexBindingDescription
  };
  if (meshFeatures.flags.test(MeshFeatures::IsInstanced)) {
    m_vertexBindingDescriptions.push_back(VkVertexInputBindingDescription{
      .binding = 1,
      .stride = sizeof(MeshInstance),
      .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    });
  }

  m_vertexInputStateInfo = VkPipelineVertexInputStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexBindingDescriptions.size()),
    .pVertexBindingDescriptions = m_vertexBindingDescriptions.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data()
  };

  m_inputAssemblyStateInfo = defaultInputAssemblyState();
  m_rasterizationStateInfo =
    defaultRasterizationState(materialFeatures.flags.test(MaterialFeatures::IsDoubleSided));
  if (m_renderPass == RenderPass::Shadow) {
    m_rasterizationStateInfo.depthBiasEnable = VK_TRUE;
    m_rasterizationStateInfo.depthClampEnable = VK_FALSE;
    m_rasterizationStateInfo.depthBiasClamp = 0.f;
    m_rasterizationStateInfo.depthBiasConstantFactor = 0.f;
    m_rasterizationStateInfo.depthBiasSlopeFactor = 0.1f;
    m_rasterizationStateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
  }
  m_multisampleStateInfo = defaultMultisamplingState();
  m_colourBlendStateInfo = defaultColourBlendState(m_colourBlendAttachmentState);
  m_depthStencilStateInfo = defaultDepthStencilState();

  m_descriptorSetLayouts = {
    m_renderResources.getDescriptorSetLayout(DescriptorSetNumber::Global),
    m_renderResources.getDescriptorSetLayout(DescriptorSetNumber::RenderPass),
    m_renderResources.getDescriptorSetLayout(DescriptorSetNumber::Material),
    m_renderResources.getDescriptorSetLayout(DescriptorSetNumber::Object)
  };

  if (!meshFeatures.flags.test(MeshFeatures::IsInstanced)
    && !meshFeatures.flags.test(MeshFeatures::IsSkybox)) {

    m_pushConstantRanges = {
      VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(Mat4x4f)
      }
    };
  }

  m_layoutInfo = VkPipelineLayoutCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size()),
    .pSetLayouts = m_descriptorSetLayouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size()),
    .pPushConstantRanges = m_pushConstantRanges.size() == 0 ? nullptr : m_pushConstantRanges.data()
  };

  switch (m_renderPass) {
    case RenderPass::Main:
      m_renderingCreateInfo = VkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_swapchainImageFormat,
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
      };
      break;
    case RenderPass::Shadow:
      m_renderingCreateInfo = VkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
      };
      break;
  }

  constructPipeline(swapchainExtent);
}

void PipelineImpl::constructPipeline(VkExtent2D swapchainExtent)
{
  destroyPipeline();

  VK_CHECK(vkCreatePipelineLayout(m_device, &m_layoutInfo, nullptr, &m_layout),
    "Failed to create default pipeline layout");

  VkPipelineShaderStageCreateInfo shaderStages[] = {
    m_vertShaderStageInfo,
    m_fragShaderStageInfo
  };

  auto viewportStateInfo = defaultViewportState(m_viewport, m_scissor, swapchainExtent);

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &m_renderingCreateInfo,
    .flags = 0,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &m_vertexInputStateInfo,
    .pInputAssemblyState = &m_inputAssemblyStateInfo,
    .pTessellationState = nullptr,
    .pViewportState = &viewportStateInfo,
    .pRasterizationState = &m_rasterizationStateInfo,
    .pMultisampleState = &m_multisampleStateInfo,
    .pDepthStencilState = &m_depthStencilStateInfo,
    .pColorBlendState = &m_colourBlendStateInfo,
    .pDynamicState = nullptr,
    .layout = m_layout,
    .renderPass = nullptr,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
    &m_pipeline), "Failed to create default pipeline");
}

void PipelineImpl::onViewportResize(VkExtent2D swapchainExtent)
{
  constructPipeline(swapchainExtent);
}

void PipelineImpl::recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
  BindState& bindState, size_t currentFrame)
{
  auto globalDescriptorSet = m_renderResources.getGlobalDescriptorSet(currentFrame);
  auto renderPassDescriptorSet = m_renderResources.getRenderPassDescriptorSet(m_renderPass,
    currentFrame);
  auto materialDescriptorSet = m_renderResources.getMaterialDescriptorSet(node.material.id);
  auto objectDescriptorSet = m_renderResources.getObjectDescriptorSet(node.mesh.id, currentFrame);

  auto buffers = m_renderResources.getMeshBuffers(node.mesh.id);

  if (m_pipeline != bindState.pipeline) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  }
  std::vector<VkBuffer> vertexBuffers{ buffers.vertexBuffer };
  if (node.mesh.features.flags.test(MeshFeatures::IsInstanced)) {
    vertexBuffers.push_back(buffers.instanceBuffer);
  }
  std::vector<VkDeviceSize> offsets(vertexBuffers.size(), 0);
  vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertexBuffers.size()),
    vertexBuffers.data(), offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  std::vector<VkDescriptorSet> descriptorSets{
    globalDescriptorSet,
    renderPassDescriptorSet,
    materialDescriptorSet
  };
  if (objectDescriptorSet != VK_NULL_HANDLE) {
    descriptorSets.push_back(objectDescriptorSet);
  }

  if (descriptorSets != bindState.descriptorSets) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0,
      static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
  }
  if (!node.mesh.features.flags.test(MeshFeatures::IsInstanced)
    && !node.mesh.features.flags.test(MeshFeatures::IsSkybox)) {

    auto& defaultNode = dynamic_cast<const DefaultModelNode&>(node);

    vkCmdPushConstants(commandBuffer, m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4x4f),
      &defaultNode.modelMatrix);
  }
  if (node.mesh.features.flags.test(MeshFeatures::IsInstanced)) {
    vkCmdDrawIndexed(commandBuffer, buffers.numIndices, buffers.numInstances, 0, 0, 0);
  }
  else {
    vkCmdDrawIndexed(commandBuffer, buffers.numIndices, 1, 0, 0, 0);
  }

  bindState.pipeline = m_pipeline;
  bindState.descriptorSets = descriptorSets;
}

ShaderProgram PipelineImpl::compileShaderProgram(RenderPass renderPass,
  const MeshFeatureSet& meshFeatures, const MaterialFeatureSet& materialFeatures)
{
  std::vector<std::string> defines;
  for (auto attr : meshFeatures.vertexLayout) {
    switch (attr) {
      case BufferUsage::AttrPosition: defines.push_back("ATTR_POSITION"); break;
      case BufferUsage::AttrNormal: defines.push_back("ATTR_NORMAL"); break;
      case BufferUsage::AttrTexCoord: defines.push_back("ATTR_TEXCOORD"); break;
      case BufferUsage::AttrTangent: defines.push_back("ATTR_TANGENT"); break;
      case BufferUsage::AttrJointIndices: defines.push_back("ATTR_JOINTS"); break;
      case BufferUsage::AttrJointWeights: defines.push_back("ATTR_WEIGHTS"); break;
      default: break;
    }
  }

  if (meshFeatures.flags.test(MeshFeatures::IsInstanced)) {
    defines.push_back("ATTR_MODEL_MATRIX");
  }
  if (meshFeatures.flags.test(MeshFeatures::IsAnimated)) {
    defines.push_back("FEATURE_VERTEX_SKINNING");
  }
  if (renderPass == RenderPass::Shadow) {
    defines.push_back("RENDER_PASS_SHADOW");
    defines.push_back("FRAG_MAIN_DEPTH");
  }
  else {
    defines.push_back("FEATURE_LIGHTING");
    defines.push_back("FEATURE_MATERIALS");

    if (meshFeatures.flags.test(MeshFeatures::IsSkybox)) {
      defines.push_back("VERT_MAIN_PASSTHROUGH");
      defines.push_back("FRAG_MAIN_SKYBOX");
    }
    if (materialFeatures.flags.test(MaterialFeatures::HasNormalMap)) {
      assert(meshFeatures.flags.test(MeshFeatures::HasTangents));
      defines.push_back("FEATURE_NORMAL_MAPPING");
    }
    if (materialFeatures.flags.test(MaterialFeatures::HasTexture)) {
      defines.push_back("FEATURE_TEXTURE_MAPPING");
    }
  }

  m_logger.info(STR("Compiling shaders with options: " << defines));
  m_logger.info(STR("Render pass: " << static_cast<int>(renderPass)));
  m_logger.info(STR("Mesh features: " << meshFeatures));
  m_logger.info(STR("Material features: " << materialFeatures));

  ShaderProgram program;

  auto vertShaderSrc = m_fileSystem.readFile("shaders/vertex/main.glsl");
  auto fragShaderSrc = m_fileSystem.readFile("shaders/fragment/main.glsl");
  program.vertexShaderCode = compileShader("vertex", vertShaderSrc, ShaderType::Vertex, defines);
  program.fragmentShaderCode = compileShader("fragment", fragShaderSrc, ShaderType::Fragment,
    defines);

  assert(program.fragmentShaderCode.size() > 0);
  assert(program.vertexShaderCode.size() > 0);

  return program;
}

std::vector<uint32_t> PipelineImpl::compileShader(const std::string& name,
  const std::vector<char>& source, ShaderType type, const std::vector<std::string>& defines)
{
  shaderc_shader_kind kind = shaderc_shader_kind::shaderc_glsl_vertex_shader;
  switch (type) {
    case ShaderType::Vertex: kind = shaderc_shader_kind::shaderc_glsl_vertex_shader; break;
    case ShaderType::Fragment: kind = shaderc_shader_kind::shaderc_glsl_fragment_shader; break;
  }

  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  options.SetWarningsAsErrors();
  options.SetIncluder(std::make_unique<SourceIncluder>(m_fileSystem));
  for (auto& define : defines) {
    options.AddMacroDefinition(define);
  }

  auto result = compiler.CompileGlslToSpv(source.data(), source.size(), kind, name.c_str(),
    options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    EXCEPTION("Error compiling shader: " << result.GetErrorMessage());
  }

  std::vector<uint32_t> code;
  code.assign(result.cbegin(), result.cend());

  return code;
}

void PipelineImpl::destroyPipeline()
{
  if (m_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
  }
  if (m_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
  }
}

PipelineImpl::~PipelineImpl()
{
  if (m_vertShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(m_device, m_vertShaderModule, nullptr);
  }
  if (m_fragShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(m_device, m_fragShaderModule, nullptr);
  }
  destroyPipeline();
}

} // namespace

PipelinePtr createPipeline(RenderPass renderPass, const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures, const FileSystem& fileSystem,
  const RenderResources& renderResources, Logger& logger, VkDevice device,
  VkExtent2D swapchainExtent, VkFormat swapchainImageFormat, VkFormat depthFormat)
{
  return std::make_unique<PipelineImpl>(renderPass, meshFeatures, materialFeatures,
    fileSystem, renderResources, logger, device, swapchainExtent, swapchainImageFormat,
    depthFormat);
}
