#include "Explorer.h"

#include <filesystem>

namespace Cosmos
{
	Explorer::Explorer(Shared<Renderer> renderer)
		: mRenderer(renderer)
	{
		mCurrentDir = GetAssetsDir();

		// folder
		mFolderRes.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/folder.png").c_str());
		mFolderRes.view.descriptor = (VkDescriptorSet)UI::AddTexture(mFolderRes.view.texture);

		// back folder
		mBackRes.path = GetAssetsDir();
		mBackRes.view.descriptor = mFolderRes.view.descriptor;
		mBackRes.view.texture = mFolderRes.view.texture;
		mBackRes.displayName = "...";

		// text
		mTextRes.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/text.png").c_str());
		mTextRes.view.descriptor = (VkDescriptorSet)UI::AddTexture(mTextRes.view.texture);

		// font
		mFont.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/font.png").c_str());
		mFont.view.descriptor = (VkDescriptorSet)UI::AddTexture(mFont.view.texture);

		// fragment shader
		mFragmentShaderRes.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/frag.png").c_str());
		mFragmentShaderRes.view.descriptor = (VkDescriptorSet)UI::AddTexture(mFragmentShaderRes.view.texture);

		// vertex shader
		mVertexShaderRes.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/vert.png").c_str());
		mVertexShaderRes.view.descriptor = (VkDescriptorSet)UI::AddTexture(mVertexShaderRes.view.texture);

		// gltf mesh
		mMeshRes.view.texture = Texture2D::Create(mRenderer, GetAssetSubDir("Texture/Editor/mesh.png").c_str());
		mMeshRes.view.descriptor = (VkDescriptorSet)UI::AddTexture(mMeshRes.view.texture);
	}

	Explorer::~Explorer()
	{
		mCurrentDirAssets.clear();
	}

	void Explorer::OnUpdate()
	{
		// layout variables
		ImVec2 currentPos = {};
		const ImVec2 buttonSize = { 64, 64 };

		// refresh directory only when needed
		if (mCurrentDirRefresh)
		{
			Refresh(mCurrentDir);
			mCurrentDirRefresh = false;
		}

		ImGui::Begin("Explorer");

		// draw the go-back folder
		DrawAsset(mBackRes, currentPos, buttonSize);

		// draw assets on current folder
		for (size_t i = 0; i < mCurrentDirAssets.size(); i++)
		{
			DrawAsset(mCurrentDirAssets[i], currentPos, buttonSize);
		}

		ImGui::End();
	}

	void Explorer::DrawAsset(Asset& asset, ImVec2& position, const ImVec2 buttonSize)
	{
		constexpr float offSet = 5.0f;
		constexpr uint32_t displayNameMaxChars = 32;

		// checks if must draw in the same line
		if ((position.x + (buttonSize.x * 2)) <= ImGui::GetContentRegionAvail().x) 
			ImGui::SameLine();

		// draws group
		ImGui::BeginGroup();
		{
			// resoruce representation of the asset
			if (ImGui::ImageButton(asset.path.c_str(), asset.view.descriptor, buttonSize))
			{
				std::filesystem::path path(asset.path);
				auto dir = std::filesystem::directory_entry(path);

				// folder asset, must flag to refresh folder on next frame
				if (dir.is_directory())
				{
					mCurrentDirRefresh = true;
					mCurrentDir = dir.path().string();
				}
			}

			// display name
			position = ImGui::GetCursorPos();
			ImGui::SetCursorPosX({ position.x /*+ offSet*/ });

			// name is too large, displaying a shorter name
			if (asset.displayName.size() > displayNameMaxChars)
			{
				std::string shortName = asset.displayName.substr(0, displayNameMaxChars);
				ImGui::Text(shortName.c_str());
			}

			else
			{
				ImGui::Text(asset.displayName.c_str());
			}
		}
		ImGui::EndGroup();

		// drag and drop behaviour
		if (asset.type != Asset::Type::Folder)
		{
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s", asset.path.c_str());
				ImGui::EndTooltip();

				ImGui::SetDragDropPayload("EXPLORER", asset.path.c_str(), asset.path.size() + 1, ImGuiCond_Once);
				ImGui::EndDragDropSource();
			}
		}
	}

	void Explorer::Refresh(std::string path)
	{
		// clear previous assets
		mCurrentDirAssets.clear();

		for (const std::filesystem::directory_entry& dirEntry : std::filesystem::directory_iterator(path))
		{
			// default asset configs
			std::string path = dirEntry.path().string();
			Cosmos::replace(path.begin(), path.end(), char('\\'), char('/'));

			Asset asset = {};
			asset.path = path;
			asset.displayName = dirEntry.path().filename().replace_extension().string();

			// check if it's folder
			if (std::filesystem::is_directory(dirEntry))
			{
				asset.type = Asset::Type::Folder;
				asset.view = mFolderRes.view;
				
				mCurrentDirAssets.push_back(asset);
				continue;
			}

			// check for general supported files
			std::string ext = dirEntry.path().extension().string();

			if (strcmp(".txt", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Text;
				asset.view = mTextRes.view;

				mCurrentDirAssets.push_back(asset);
				continue;
			}

			if (strcmp(".ttf", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Font;
				asset.view = mFont.view;

				mCurrentDirAssets.push_back(asset);
				continue;
			}

			if (strcmp(".frag", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Fragment;
				asset.view = mFragmentShaderRes.view;

				mCurrentDirAssets.push_back(asset);
				continue;
			}

			if (strcmp(".vert", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Vertex;
				asset.view = mVertexShaderRes.view;

				mCurrentDirAssets.push_back(asset);
				continue;
			}

			if (strcmp(".gltf", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Mesh;
				asset.view = mMeshRes.view;

				mCurrentDirAssets.push_back(asset);
				continue;
			}

			if (strcmp(".png", ext.c_str()) == 0 || strcmp(".jpg", ext.c_str()) == 0)
			{
				asset.type = Asset::Type::Image;
				asset.view.texture = Texture2D::Create(mRenderer, asset.path.c_str());
				asset.view.descriptor = (VkDescriptorSet)UI::AddTexture(asset.view.texture);

				mCurrentDirAssets.push_back(asset);
				continue;
			}
		}
	}
}