#pragma once

#define COSMOS_RENDERER_VULKAN
#include <Engine.h>

#include <map>
#include <Renderer/Vulkan/Shader.h>

namespace Cosmos
{
	class Explorer : public Widget
	{
	public:

		struct Asset
		{
			enum Type : unsigned int
			{
				Undefined,
				Folder,
				Text,
				Font,
				Fragment,
				Vertex,
				Mesh,
				Image
			};

			typedef struct View
			{
				Shared<Texture2D> texture = {};
				VkDescriptorSet descriptor = VK_NULL_HANDLE;
			} View;

			View view = {};
			Type type = Undefined;
			std::string path = {};
			std::string displayName = {};
		};

	public:

		// constructor
		Explorer(Shared<Renderer> renderer);

		// destructor
		~Explorer();

	public:

		// updates the tick logic
		virtual void OnUpdate() override;

	private:

		// draws a folder in the explorer
		void DrawAsset(Asset& asset, ImVec2& position, const ImVec2 buttonSize);

		// reloads the folder's content
		void Refresh(std::string path);

	private:

		Shared<Renderer> mRenderer;

		bool mCurrentDirRefresh = true;
		std::string mCurrentDir = {};
		std::vector<Asset> mCurrentDirAssets = {};

		// default view resources
		Asset mBackRes;
		Asset mFolderRes;
		Asset mTextRes;
		Asset mFont;
		Asset mFragmentShaderRes;
		Asset mVertexShaderRes;
		Asset mMeshRes;
	};
}