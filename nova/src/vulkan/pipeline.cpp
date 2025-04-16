#include "vulkan/pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"
#include "vulkan/render_resources.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include "model.hpp"
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
  createAttributeDescriptions(const std::vector<BufferUsage> layout)
{
  std::vector<VkVertexInputAttributeDescription> attributes;

  const uint32_t first = static_cast<uint32_t>(BufferUsage::AttrPosition);
  for (auto& attribute : layout) {
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

VkPipelineRasterizationStateCreateInfo defaultRasterizationState()
{
  return VkPipelineRasterizationStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,//VK_CULL_MODE_BACK_BIT,
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

struct PipelineProperties
{
  bool hasLighting = false;
};

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
    PipelineImpl(const MeshFeatureSet& meshFeatures, const MaterialFeatureSet& materialFeatures,
      const FileSystem& fileSystem, const RenderResources& renderResources, VkDevice device,
      VkExtent2D swapchainExtent, VkRenderPass renderPass);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      BindState& bindState, size_t currentFrame) override;

    ~PipelineImpl() override;

  private:
    const FileSystem& m_fileSystem;
    const RenderResources& m_renderResources;
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    PipelineProperties m_properties; // TODO: Remove?

    ShaderProgram compileShaderProgram(const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures);

    std::vector<uint32_t> compileShader(const std::string& name, const std::vector<char>& source,
      ShaderType type, const std::vector<std::string>& defines);
};

ShaderProgram PipelineImpl::compileShaderProgram(const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures)
{
  std::vector<std::string> defines;
  if (meshFeatures.isInstanced) {
    defines.push_back("INSTANCED");
  }
  if (meshFeatures.isSkybox) {
    defines.push_back("SKYBOX");
  }
  if (materialFeatures.hasNormalMap) {
    assert(meshFeatures.hasTangents);
    defines.push_back("NORMAL_MAPPING");
  }
  if (materialFeatures.hasTexture) {
    defines.push_back("TEXTURE_MAPPING");
  }

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

PipelineImpl::PipelineImpl(const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures, const FileSystem& fileSystem,
  const RenderResources& renderResources, VkDevice device, VkExtent2D swapchainExtent,
  VkRenderPass renderPass)
  : m_fileSystem(fileSystem)
  , m_renderResources(renderResources)
  , m_device(device)
{
  auto program = compileShaderProgram(meshFeatures, materialFeatures);

  VkShaderModule vertShaderModule = createShaderModule(m_device, program.vertexShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(m_device, program.fragmentShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName = "main",
    .pSpecializationInfo = nullptr
  };

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
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

  auto attributeDescriptions = createAttributeDescriptions(meshFeatures.vertexLayout);
  if (meshFeatures.isInstanced) {
    for (unsigned int i = 0; i < 4; ++i) {
      uint32_t offset = offsetof(MeshInstance, modelMatrix) + 4 * sizeof(float_t) * i;
  
      VkVertexInputAttributeDescription attr{
        .location = 6 + i, // TODO
        .binding = 1,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offset
      };
  
      attributeDescriptions.push_back(attr);
    }
  }

  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
    vertexBindingDescription
  };
  if (meshFeatures.isInstanced) {
    bindingDescriptions.push_back(VkVertexInputBindingDescription{
      .binding = 1,
      .stride = sizeof(MeshInstance),
      .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    });
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
    .pVertexBindingDescriptions = bindingDescriptions.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data()
  };

  auto inputAssembly = defaultInputAssemblyState();
  VkViewport viewport;
  VkRect2D scissor;
  auto viewportState = defaultViewportState(viewport, scissor, swapchainExtent);
  auto rasterizer = defaultRasterizationState();
  auto multisampling = defaultMultisamplingState();
  VkPipelineColorBlendAttachmentState colourBlendAttachment;
  auto colourBlending = defaultColourBlendState(colourBlendAttachment);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
    m_renderResources.getMatricesDescriptorSetLayout(),
    m_renderResources.getMaterialDescriptorSetLayout()
  };
  if (!meshFeatures.isSkybox) {
    descriptorSetLayouts.push_back(m_renderResources.getLightingDescriptorSetLayout());
  }

  std::vector<VkPushConstantRange> pushConstantRanges;
  if (!meshFeatures.isInstanced && !meshFeatures.isSkybox) {
    pushConstantRanges = {
      VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(Mat4x4f)
      }
    };
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
    .pPushConstantRanges = pushConstantRanges.size() == 0 ? nullptr : pushConstantRanges.data()
  };

  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout),
    "Failed to create default pipeline layout");

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto depthStencil = defaultDepthStencilState();

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pTessellationState = nullptr,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colourBlending,
    .pDynamicState = nullptr,
    .layout = m_layout,
    .renderPass = renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
    &m_pipeline), "Failed to create default pipeline");

  vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void PipelineImpl::recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
  BindState& bindState, size_t currentFrame)
{
  auto matricesDescriptorSet = m_renderResources.getMatricesDescriptorSet(currentFrame);
  auto materialDescriptorSet = m_renderResources.getMaterialDescriptorSet(node.material.id);
  auto lightingDescriptorSet = m_renderResources.getLightingDescriptorSet(currentFrame);
  auto buffers = m_renderResources.getMeshBuffers(node.mesh.id);

  if (m_pipeline != bindState.pipeline) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  }
  std::vector<VkBuffer> vertexBuffers{ buffers.vertexBuffer };
  if (node.mesh.features.isInstanced) {
    vertexBuffers.push_back(buffers.instanceBuffer);
  }
  std::vector<VkDeviceSize> offsets(vertexBuffers.size(), 0);
  vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertexBuffers.size()),
    vertexBuffers.data(), offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  std::vector<VkDescriptorSet> descriptorSets{
    matricesDescriptorSet,
    materialDescriptorSet,
  };
  if (!node.mesh.features.isSkybox) {
    descriptorSets.push_back(lightingDescriptorSet);
  }
  if (descriptorSets != bindState.descriptorSets) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0,
      static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
  }
  if (!node.mesh.features.isInstanced && !node.mesh.features.isSkybox) {
    auto& defaultNode = dynamic_cast<const DefaultModelNode&>(node);

    vkCmdPushConstants(commandBuffer, m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4x4f),
      &defaultNode.modelMatrix);
  }
  if (node.mesh.features.isInstanced) {
    vkCmdDrawIndexed(commandBuffer, buffers.numIndices, buffers.numInstances, 0, 0, 0);
  }
  else {
    vkCmdDrawIndexed(commandBuffer, buffers.numIndices, 1, 0, 0, 0);
  }

  bindState.pipeline = m_pipeline;
  bindState.descriptorSets = descriptorSets;
}

PipelineImpl::~PipelineImpl()
{
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}

} // namespace

PipelinePtr createPipeline(const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures, const FileSystem& fileSystem,
  const RenderResources& renderResources, VkDevice device, VkExtent2D swapchainExtent,
  VkRenderPass renderPass)
{
  return std::make_unique<PipelineImpl>(meshFeatures, materialFeatures, fileSystem, renderResources,
    device, swapchainExtent, renderPass);
}
