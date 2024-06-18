#include "SceneHierarchy.h"

#include <filesystem>

namespace Cosmos
{
	SceneHierarchy::SceneHierarchy(Shared<Renderer> renderer, Shared<Scene> scene)
		: mRenderer(renderer), mScene(scene)
	{
	}

	SceneHierarchy::~SceneHierarchy()
	{
	}

	void SceneHierarchy::OnUpdate()
	{
		DisplaySceneHierarchy();
		DisplaySelectedEntityComponents();
	}

	void SceneHierarchy::DisplaySceneHierarchy()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar | ImGuiTreeNodeFlags_OpenOnArrow;

		ImGui::Begin("Entity", 0, flags);

		if (ImGui::BeginMenuBar())
		{
			constexpr float itemSize = 25.0f;
			float itemCount = 1.0f;

			ImGui::Text(ICON_FA_PAINT_BRUSH " Edit");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (itemSize * itemCount));

			if (ImGui::MenuItem(ICON_FA_PLUS_SQUARE))
			{
				mScene->CreateEntity("Entity");
			}

			ImGui::EndMenuBar();
		}

		for (auto& ent : mScene->GetEntityMapRef())
		{
			bool redraw = false;
			DrawEntityNode(ent.second, &redraw);

			if (redraw) break;
		}
		
		ImGui::End();
	}

	void SceneHierarchy::DrawEntityNode(Shared<Entity> entity, bool* redraw)
	{
		if (entity == nullptr) return;

		// create unique context
		ImGui::PushID((void*)(uint64_t)entity->GetID());

		bool selected = (mSelectedEntity == entity) ? true : false;

		if (ImGui::Selectable(entity->GetComponent<NameComponent>().name.c_str(), selected, ImGuiSelectableFlags_DontClosePopups))
		{
			mSelectedEntity = entity;
		}

		if (selected)
		{
			if (ImGui::BeginPopupContextItem("##RightClickPopup", ImGuiPopupFlags_MouseButtonRight))
			{
				if (ImGui::MenuItem("Destroy Entity"))
				{
					mScene->DestroyEntity(mSelectedEntity);
					mSelectedEntity = nullptr;
					*redraw = true;
				}

				ImGui::EndPopup();
			}
		}

		ImGui::PopID();
	}

	void SceneHierarchy::DisplaySelectedEntityComponents()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar;

		ImGui::Begin("Components", 0, flags);

		// only show components of the selected entity
		if (!mSelectedEntity)
		{
			ImGui::End();
			return;
		}

		// display edit menu
		if (ImGui::BeginMenuBar())
		{
			constexpr float itemSize = 35.0f;
			float itemCount = 1.0f;

			ImGui::Text(ICON_FA_PAINT_BRUSH " Edit Components");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (itemSize * itemCount));
			itemCount--;

			if (ImGui::BeginMenu(ICON_FA_PLUS_SQUARE))
			{
				DisplayAddComponentEntry<TransformComponent>("Transform");
				DisplayAddComponentEntry<MeshComponent>("Mesh");

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		DrawComponents(mSelectedEntity);
		ImGui::End();
	}

	void SceneHierarchy::DrawComponents(Shared<Entity> entity)
	{
		if (entity == nullptr) return;

		// general info
		ImGui::Separator();
		
		ImGui::Text("ID: %d", entity->GetID().GetValue());

		ImGui::Text("Name: ");
		ImGui::SameLine();

		constexpr unsigned int EntityNameMaxChars = 32;
		auto& name = entity->GetComponent<NameComponent>().name;
		char buffer[EntityNameMaxChars];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, name.c_str(), sizeof(buffer));

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 2.0f));
		if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
		{
			name = std::string(buffer);
		}
		ImGui::PopStyleVar();

		// 3d world-position
		DrawComponent<TransformComponent>("Transform", mSelectedEntity, [&](TransformComponent& component)
			{
				ImGui::Text("T: ");
				ImGui::SameLine();
				UI::Vector3Control("Translation", component.translation);
				
				ImGui::Text("R: ");
				ImGui::SameLine();
				glm::vec3 rotation = glm::degrees(component.rotation);
				UI::Vector3Control("Rotation", rotation);
				component.rotation = glm::radians(rotation);
				
				ImGui::Text("S: ");
				ImGui::SameLine();
				UI::Vector3Control("Scale", component.scale);
			});

		DrawComponent<MeshComponent>("Mesh", mSelectedEntity, [&](MeshComponent& component)
			{
				if (component.mesh == nullptr) {
					component.mesh = Mesh::Create(mRenderer);
				}

				// path
				constexpr unsigned int EntityNameMaxChars = 32;
				auto modelPath = component.mesh->GetFilepath();
				char buffer[EntityNameMaxChars];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, modelPath.c_str(), sizeof(buffer));
				ImGui::InputTextWithHint("", "drop from explorer", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EXPLORER"))
					{
						std::filesystem::path path = (const char*)payload->Data;
						component.mesh->LoadFromFile(path.string());
					}

					ImGui::EndDragDropTarget();
				}
			});
	}

	template<typename T>
	inline void SceneHierarchy::DisplayAddComponentEntry(const char* name)
	{
		if (ImGui::MenuItem(name))
		{
			if (!mSelectedEntity->HasComponent<T>())
			{
				mSelectedEntity->AddComponent<T>();
				return;
			}
		
			COSMOS_LOG(Logger::Error, "Entity %s already have the component %s", mSelectedEntity->GetComponent<NameComponent>().name.c_str(), name);
		}
	}

	template<typename T, typename F>
	void SceneHierarchy::DrawComponent(const char* name, Shared<Entity> entity, F func)
	{
		if (entity == nullptr) return;
		
		if (entity->HasComponent<T>())
		{
			auto& component = entity->GetComponent<T>();
			if (ImGui::TreeNodeEx((void*)typeid(T).hash_code(), 0, name))
			{
				if (ImGui::BeginPopupContextItem("##RightClickComponent", ImGuiPopupFlags_MouseButtonRight))
				{
					if (ImGui::MenuItem("Remove Component"))
					{
						entity->RemoveComponent<T>();
					}

					ImGui::EndPopup();
				}

				func(component);
		
				ImGui::TreePop();
			}
		}
	}
}