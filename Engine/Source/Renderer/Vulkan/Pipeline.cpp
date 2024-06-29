#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Pipeline.h"
#include "Device.h"
#include "Renderpass.h"
#include "Shader.h"
#include "Renderer/Buffer.h"
#include "Util/Files.h"
#include "Util/Logger.h"

namespace Cosmos::Vulkan
{
    Pipeline::Pipeline(Shared<Device> device, Specification specification)
        : mDevice(device), mSpecification(specification)
    {
        // descriptor set and pipeline layout
        VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
        descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descSetLayoutCI.pNext = nullptr;
        descSetLayoutCI.flags = 0;
        descSetLayoutCI.bindingCount = (uint32_t)mSpecification.bindings.size();
        descSetLayoutCI.pBindings = mSpecification.bindings.data();
        COSMOS_ASSERT(vkCreateDescriptorSetLayout(mDevice->GetLogicalDevice(), &descSetLayoutCI, nullptr, &mDescriptorSetLayout) == VK_SUCCESS, "Failed to create descriptor set layout");

        VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext = nullptr;
        pipelineLayoutCI.flags = 0;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &mDescriptorSetLayout;
        pipelineLayoutCI.pushConstantRangeCount = (uint32_t)mSpecification.pushConstants.size();
        pipelineLayoutCI.pPushConstantRanges = mSpecification.pushConstants.data();
        COSMOS_ASSERT(vkCreatePipelineLayout(mDevice->GetLogicalDevice(), &pipelineLayoutCI, nullptr, &mPipelineLayout) == VK_SUCCESS, "Failed to create pipeline layout");

        // shader stages
        mSpecification.shaderStagesCI =
        {
            mSpecification.vertexShader->GetShaderStageCreateInfoRef(),
            mSpecification.fragmentShader->GetShaderStageCreateInfoRef()
        };

        // pipeline default configuration
        // vertex input state
        mSpecification.VISCI = GetPipelineVertexInputState(mSpecification.vertexComponents);

        // input assembly state
        mSpecification.IASCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        mSpecification.IASCI.pNext = nullptr;
        mSpecification.IASCI.flags = 0;
        mSpecification.IASCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mSpecification.IASCI.primitiveRestartEnable = VK_FALSE;

        // viewport state
        mSpecification.VSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        mSpecification.VSCI.pNext = nullptr;
        mSpecification.VSCI.flags = 0;
        mSpecification.VSCI.viewportCount = 1;
        mSpecification.VSCI.pViewports = nullptr; // using dynamic viewport
        mSpecification.VSCI.scissorCount = 1;
        mSpecification.VSCI.pScissors = nullptr; // using dynamic scissor

        // rasterization state
        mSpecification.RSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        mSpecification.RSCI.pNext = nullptr;
        mSpecification.RSCI.flags = 0;
        mSpecification.RSCI.polygonMode = VK_POLYGON_MODE_FILL;
        mSpecification.RSCI.cullMode = VK_CULL_MODE_NONE;
        mSpecification.RSCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        mSpecification.RSCI.depthClampEnable = VK_FALSE;
        mSpecification.RSCI.rasterizerDiscardEnable = VK_FALSE;
        mSpecification.RSCI.lineWidth = 1.0f;

        // multisampling state
        mSpecification.MSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        mSpecification.MSCI.pNext = nullptr;
        mSpecification.MSCI.flags = 0;
        mSpecification.MSCI.rasterizationSamples = mSpecification.renderPass->GetSpecificationRef().msaa;
        mSpecification.MSCI.sampleShadingEnable = VK_FALSE;

        // depth stencil state
        mSpecification.DSSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        mSpecification.DSSCI.pNext = nullptr;
        mSpecification.DSSCI.flags = 0;
        mSpecification.DSSCI.depthTestEnable = VK_TRUE;
        mSpecification.DSSCI.depthWriteEnable = VK_TRUE;
        mSpecification.DSSCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        mSpecification.DSSCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

        // color blend
        mSpecification.CBAS.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        mSpecification.CBAS.blendEnable = VK_FALSE;
        mSpecification.CBAS.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        mSpecification.CBAS.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        mSpecification.CBAS.colorBlendOp = VK_BLEND_OP_ADD;
        mSpecification.CBAS.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        mSpecification.CBAS.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        mSpecification.CBAS.alphaBlendOp = VK_BLEND_OP_ADD;

        // color blend state
        mSpecification.CBSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        mSpecification.CBSCI.pNext = nullptr;
        mSpecification.CBSCI.flags = 0;
        mSpecification.CBSCI.attachmentCount = 1;
        mSpecification.CBSCI.pAttachments = &mSpecification.CBAS;
        mSpecification.CBSCI.logicOpEnable = VK_FALSE;
        mSpecification.CBSCI.logicOp = VK_LOGIC_OP_COPY;
        mSpecification.CBSCI.blendConstants[0] = 0.0f;
        mSpecification.CBSCI.blendConstants[1] = 0.0f;
        mSpecification.CBSCI.blendConstants[2] = 0.0f;
        mSpecification.CBSCI.blendConstants[3] = 0.0f;

        // dynamic state
        mSpecification.DSCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        mSpecification.DSCI.pNext = nullptr;
        mSpecification.DSCI.flags = 0;
        mSpecification.DSCI.dynamicStateCount = (uint32_t)mSpecification.dynamicStates.size();
        mSpecification.DSCI.pDynamicStates = mSpecification.dynamicStates.data();
    }

    Pipeline::~Pipeline()
    {
        vkDeviceWaitIdle(mDevice->GetLogicalDevice());

        vkDestroyPipeline(mDevice->GetLogicalDevice(), mPipeline, nullptr);
        vkDestroyPipelineLayout(mDevice->GetLogicalDevice(), mPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(mDevice->GetLogicalDevice(), mDescriptorSetLayout, nullptr);
    }

    void Pipeline::Build(VkPipelineCache cache)
    {
        // pipeline creation
        VkGraphicsPipelineCreateInfo pipelineCI = {};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.pNext = nullptr;
        pipelineCI.flags = 0;
        pipelineCI.stageCount = (uint32_t)mSpecification.shaderStagesCI.size();
        pipelineCI.pStages = mSpecification.shaderStagesCI.data();
        pipelineCI.pVertexInputState = &mSpecification.VISCI;
        pipelineCI.pInputAssemblyState = &mSpecification.IASCI;
        pipelineCI.pViewportState = &mSpecification.VSCI;
        pipelineCI.pRasterizationState = &mSpecification.RSCI;
        pipelineCI.pMultisampleState = &mSpecification.MSCI;
        pipelineCI.pDepthStencilState = &mSpecification.DSSCI;
        pipelineCI.pColorBlendState = &mSpecification.CBSCI;
        pipelineCI.pDynamicState = &mSpecification.DSCI;
        pipelineCI.layout = mPipelineLayout;
        pipelineCI.renderPass = mSpecification.renderPass->GetSpecificationRef().renderPass;
        pipelineCI.subpass = 0;
        COSMOS_ASSERT(vkCreateGraphicsPipelines(mDevice->GetLogicalDevice(), cache, 1, &pipelineCI, nullptr, &mPipeline) == VK_SUCCESS, "Failed to create graphics pipeline");
    }

    std::vector<VkVertexInputBindingDescription> Pipeline::GetBindingDescriptions()
    {
        if (mSpecification.notPassingVertexData)
            return std::vector<VkVertexInputBindingDescription>{};

        mBindingDescriptions.clear();
        mBindingDescriptions.resize(1);

        mBindingDescriptions[0].binding = 0;
        mBindingDescriptions[0].stride = sizeof(Vertex);
        mBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return mBindingDescriptions;
    }

    VkVertexInputAttributeDescription Pipeline::GetInputAttributeDescription(uint32_t binding, uint32_t location, Vertex::Component component)
    {
        switch (component)
        {
            case Vertex::Component::POSITION: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
            case Vertex::Component::NORMAL: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
            case Vertex::Component::UV: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });
            case Vertex::Component::JOINT: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_UINT, offsetof(Vertex, joint) });
            case Vertex::Component::WEIGHT: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight) });
            case Vertex::Component::COLOR: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });
            case Vertex::Component::UID: return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32_UINT, offsetof(Vertex, uid) });
        }
        
        return VkVertexInputAttributeDescription({});
    }

    std::vector<VkVertexInputAttributeDescription> Pipeline::GetAttributeDescriptions(const std::vector<Vertex::Component> components)
    {
        std::vector<VkVertexInputAttributeDescription> result = {};
        constexpr uint32_t binding = 0;
        uint32_t location = 0;

        for (auto component : components)
        {
            switch (component)
            {
                case Vertex::Component::POSITION: location = 0; break;
                case Vertex::Component::NORMAL: location = 1; break;
                case Vertex::Component::UV: location = 2; break;
                case Vertex::Component::JOINT: location = 3; break;
                case Vertex::Component::WEIGHT: location = 4; break;
                case Vertex::Component::COLOR: location = 5; break;
                case Vertex::Component::UID: location = 6; break;
            }

            result.push_back(GetInputAttributeDescription(binding, location, component));
        }

        return result;
    }

    VkPipelineVertexInputStateCreateInfo Pipeline::GetPipelineVertexInputState(const std::vector<Vertex::Component> components)
    {
        mBindingDescriptions = GetBindingDescriptions();
        mAttributeDescriptions = GetAttributeDescriptions(components);

        VkPipelineVertexInputStateCreateInfo VISCI = {};
        VISCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VISCI.pNext = nullptr;
        VISCI.flags = 0;
        VISCI.vertexBindingDescriptionCount = (uint32_t)mBindingDescriptions.size();
        VISCI.pVertexBindingDescriptions = mBindingDescriptions.data();
        VISCI.vertexAttributeDescriptionCount = (uint32_t)mAttributeDescriptions.size();
        VISCI.pVertexAttributeDescriptions = mAttributeDescriptions.data();

        return VISCI;
    }

	PipelineLibrary::PipelineLibrary(Shared<Device> device, Shared<RenderpassManager> renderpassManager)
		: mDevice(device), mRenderpassManager(renderpassManager)
	{
        VkPipelineCacheCreateInfo pipelineCacheCI = {};
        pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCI.pNext = nullptr;
        pipelineCacheCI.flags = 0;
        COSMOS_ASSERT(vkCreatePipelineCache(mDevice->GetLogicalDevice(), &pipelineCacheCI, nullptr, &mCache) == VK_SUCCESS, "Failed to create pipeline cache");
	
        CreateMeshPipeline();
        CreateSkyboxPipeline();
    }

	PipelineLibrary::~PipelineLibrary()
	{
        vkDestroyPipelineCache(mDevice->GetLogicalDevice(), mCache, nullptr);
	}

    void PipelineLibrary::RecreatePipelines()
    {
        CreateMeshPipeline();
        CreateSkyboxPipeline();
    }

    void PipelineLibrary::Insert(const char* nameid, Shared<Pipeline> pipeline)
    {
        auto it = mPipelines.find(nameid);

        if (it != mPipelines.end())
        {
            COSMOS_LOG(Logger::Error, "A Pipeline with name %s already exists, could not create another", nameid);
            return;
        }

        mPipelines[nameid] = pipeline;
    }

    void PipelineLibrary::Erase(const char* nameid)
    {
        auto it = mPipelines.find(nameid);

        if (it != mPipelines.end())
        {
            mPipelines.erase(nameid);
            return;
        }

        COSMOS_LOG(Logger::Error, "No Pipeline with name %s exists", nameid);
        return;
    }

    void PipelineLibrary::CreateMeshPipeline()
    {
        Pipeline::Specification meshSpecification = {};
        meshSpecification.renderPass = mRenderpassManager->GetMainRenderpass();
        meshSpecification.vertexShader = CreateShared<Shader>(mDevice, Shader::Type::Vertex, "Mesh.vert", GetAssetSubDir("Shader/mesh.vert"));
        meshSpecification.fragmentShader = CreateShared<Shader>(mDevice, Shader::Type::Fragment, "Mesh.frag", GetAssetSubDir("Shader/mesh.frag"));
        meshSpecification.vertexComponents = 
        {
            Vertex::Component::POSITION,
            Vertex::Component::NORMAL,
            Vertex::Component::UV
        };

        VkPushConstantRange pushConstant = {};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(CameraBuffer);
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        meshSpecification.pushConstants.push_back(pushConstant);
        
        meshSpecification.bindings.resize(2);
        
        // camera ubo
        meshSpecification.bindings[0].binding = 0;
        meshSpecification.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        meshSpecification.bindings[0].descriptorCount = 1;
        meshSpecification.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        meshSpecification.bindings[0].pImmutableSamplers = nullptr;

        // color map
        meshSpecification.bindings[1].binding = 3;
        meshSpecification.bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        meshSpecification.bindings[1].descriptorCount = 1;
        meshSpecification.bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        meshSpecification.bindings[1].pImmutableSamplers = nullptr;
        
        // common
        {
            // create normal pipeline
            mPipelines["Mesh.Common"] = CreateShared<Pipeline>(mDevice, meshSpecification);
        
            // modify parameters after initial creation
            mPipelines["Mesh.Common"]->GetSpecificationRef().RSCI.cullMode = VK_CULL_MODE_BACK_BIT;
        
            // build the pipeline
            mPipelines["Mesh.Common"]->Build(mCache);
        }
        
        // wireframed
        {
            // create
            mPipelines["Mesh.Wireframed"] = CreateShared<Pipeline>(mDevice, meshSpecification);
        
            // modify parameters after initial creation
            mPipelines["Mesh.Wireframed"]->GetSpecificationRef().RSCI.cullMode = VK_CULL_MODE_BACK_BIT;
            mPipelines["Mesh.Wireframed"]->GetSpecificationRef().RSCI.polygonMode = VK_POLYGON_MODE_LINE;
            mPipelines["Mesh.Wireframed"]->GetSpecificationRef().RSCI.lineWidth = 5.0f;
        
            // build the pipeline
            mPipelines["Mesh.Wireframed"]->Build(mCache);
        }
    }

    void PipelineLibrary::CreateSkyboxPipeline()
    {
        Pipeline::Specification skyboxSpecification = {};
        skyboxSpecification.renderPass = mRenderpassManager->GetMainRenderpass();
        skyboxSpecification.vertexShader = CreateShared<Shader>(mDevice, Shader::Type::Vertex, "Skybox.vert", GetAssetSubDir("Shader/skybox.vert"));
        skyboxSpecification.fragmentShader = CreateShared<Shader>(mDevice, Shader::Type::Fragment, "Skybox.frag", GetAssetSubDir("Shader/skybox.frag"));
        skyboxSpecification.vertexComponents = { Vertex::Component::POSITION };

        VkPushConstantRange pushConstant = {};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(ObjectPushConstant);
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        skyboxSpecification.pushConstants.push_back(pushConstant);

        skyboxSpecification.bindings.resize(2);

        // model view projection
        skyboxSpecification.bindings[0].binding = 0;
        skyboxSpecification.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxSpecification.bindings[0].descriptorCount = 1;
        skyboxSpecification.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        skyboxSpecification.bindings[0].pImmutableSamplers = nullptr;

        // cubemap
        skyboxSpecification.bindings[1].binding = 1;
        skyboxSpecification.bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxSpecification.bindings[1].descriptorCount = 1;
        skyboxSpecification.bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        skyboxSpecification.bindings[1].pImmutableSamplers = nullptr;

        // create
        mPipelines["Skybox"] = CreateShared<Pipeline>(mDevice, skyboxSpecification);

        // modify parameters after initial creation
        mPipelines["Skybox"]->GetSpecificationRef().RSCI.cullMode = VK_CULL_MODE_FRONT_BIT;

        // build the pipeline
        mPipelines["Skybox"]->Build(mCache);
    }
}

#endif