#pragma once

#include "Renderer/Vertex.h"
#include "Util/Memory.h"
#include <volk.h>

#include <unordered_map>

namespace Cosmos::Vulkan
{
    // forward declarations
    class Device;
    class Renderpass;
    class RenderpassManager;
    class Shader;

    class Pipeline
    {
    public:

        struct Specification
        {
            // these must be created before creating the VKPipeline
            Shared<Renderpass> renderPass;
            Shared<Shader> vertexShader;
            Shared<Shader> fragmentShader;
            std::vector<Vertex::Component> vertexComponents = {};
            std::vector<VkDescriptorSetLayoutBinding> bindings = {};

            // these will be auto generated, but can be previously modified between Pipeline constructor and Build for customization
            std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            std::vector<VkPipelineShaderStageCreateInfo> shaderStagesCI = {};
            VkPipelineVertexInputStateCreateInfo VISCI = {};
            VkPipelineInputAssemblyStateCreateInfo IASCI = {};
            VkPipelineViewportStateCreateInfo VSCI = {};
            VkPipelineRasterizationStateCreateInfo RSCI = {};
            VkPipelineMultisampleStateCreateInfo MSCI = {};
            VkPipelineDepthStencilStateCreateInfo DSSCI = {};
            VkPipelineColorBlendAttachmentState CBAS = {};
            VkPipelineColorBlendStateCreateInfo CBSCI = {};
            VkPipelineDynamicStateCreateInfo DSCI = {};
        };

    public:

        // constructor with a previously defined specification
        Pipeline(Shared<Device> device, Specification specification);

        // destructor
        ~Pipeline();

        // returns a reference to the pipeline specification
        inline Specification& GetSpecificationRef() { return mSpecification; }

        // returns the descriptor set layout
        inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return mDescriptorSetLayout; }

        // returns the pipeline layout
        inline VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }

        // returns the pipeline
        inline VkPipeline GetPipeline() const { return mPipeline; }

    public:

        // creates a pipeline object given previously configured struct VKPiplineSpecification, cache may be nullptr
        void Build(VkPipelineCache cache);

    private:

        // returns the binding descriptions, currently it is very simple and only has one binding, used internally
        std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();

        // returns the attribute description based on configuration, used internally
        VkVertexInputAttributeDescription GetInputAttributeDescription(uint32_t binding, uint32_t location, Vertex::Component component);

        // returns the attribute descriptions, it generates the attributes based on a list of desired attributes, used internally
        std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(const std::vector<Vertex::Component> components);

        // returns the pipeline vertex input state based on desired components
        VkPipelineVertexInputStateCreateInfo GetPipelineVertexInputState(const std::vector<Vertex::Component> components);

    private:

        Shared<Device> mDevice;
        Specification mSpecification;
        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
        VkPipeline mPipeline = VK_NULL_HANDLE;

        // these variables must be hold in memory for the creation of pre-configured pipelines
        std::vector<VkVertexInputBindingDescription> mBindingDescriptions = {};
        std::vector<VkVertexInputAttributeDescription> mAttributeDescriptions = {};
    };

	class PipelineLibrary
	{
	public:

		// constructor
		PipelineLibrary(Shared<Device> device, Shared<RenderpassManager> renderpassManager);

		// destructor
		~PipelineLibrary();

		// returns the pipeline cache
		inline VkPipelineCache GetPipelineCache() { return mCache; }

        // returns a reference to the pipeline libraries
        inline std::unordered_map<std::string, Shared<Pipeline>>& GetPipelinesRef() { return mPipelines; }

        // recreate all pipelines, used when renderpass get's modified
        void RecreatePipelines();

    private:

        // create globally-used pipeline for meshes
        void CreateMeshPipeline();

        // create globally-used pipeline for skybox
        void CreateSkyboxPipeline();

	private:

		Shared<Device> mDevice;
        Shared<RenderpassManager> mRenderpassManager;
		VkPipelineCache mCache = VK_NULL_HANDLE;
		std::unordered_map<std::string, Shared<Pipeline>> mPipelines = {};
	};
}