#include "Menubar.h"

#include "Renderer/Grid.h"

namespace Cosmos
{
	Menubar::Menubar(Application* application, Shared<Window> window, Shared<Renderer> renderer, Grid* grid)
		: Widget("Menubar"), mApplication(application), mWindow(window), mRenderer(renderer), mGrid(grid)
	{

	}

	void Menubar::OnUpdate()
	{
		ImGui::BeginMainMenuBar();
		DisplayMainMenu();
		ImGui::EndMainMenuBar();

		HandleMenuAction();

		// scene info
		auto& camera = mRenderer->GetCamera();
		ImGuiWindowFlags flags = {};
		
		ImGui::Begin("Info", nullptr, flags);

		ImGui::Text(ICON_FA_INFO_CIRCLE " FPS: %d", mWindow->GetFramesPerSecond());
		ImGui::Text(ICON_FA_INFO_CIRCLE " Timestep: %f", mWindow->GetTimestep());
		ImGui::Text(ICON_FA_CAMERA " Camera Pos: %.2f %.2f %.2f", camera->GetPositionRef().x, camera->GetPositionRef().y, camera->GetPositionRef().z);
		ImGui::Text(ICON_FA_CAMERA " Camera Rot: %.2f %.2f %.2f", camera->GetRotationRef().x, camera->GetRotationRef().y, camera->GetRotationRef().z);

		ImGui::End();

		// scene settings
		SceneSettingsWindow();
	}

	void Menubar::DisplayMainMenu()
	{
		mMenuAction = Action::None;

		if (ImGui::BeginMenu(ICON_FA_FILE " Project"))
		{
			if (ImGui::MenuItem("New")) mMenuAction = Action::New;
			if (ImGui::MenuItem("Open")) mMenuAction = Action::Open;
			if (ImGui::MenuItem("Save")) mMenuAction = Action::Save;

			ImGui::Separator();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FA_BOOKMARK " View"))
		{
			if (UI::CheckboxSliderEx("Draw Grid", &mCheckboxGrid))
			{
				mGrid->ToogleOnOff();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FA_COG " Settings"))
		{
			if (ImGui::MenuItem("Scene Settings", nullptr))
			{
				mDisplaySceneSettings = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_LC_EGG " Debug"))
		{
			if (ImGui::MenuItem("Simulate Physics Test", nullptr))
			{
				mApplication->GetPhysicsWorld()->RunTest();
			}

			ImGui::EndMenu();
		}
	}

	void Menubar::HandleMenuAction()
	{
		switch (mMenuAction)
		{
			case Menubar::New:
			{
				break;
			}

			case Menubar::Open:
			{
				break;
			}

			case Menubar::Save:
			{
				break;
			}

			case Menubar::SaveAs:
			{
				break;
			}
		}
	}

	void Menubar::SceneSettingsWindow()
	{
		if (!mDisplaySceneSettings)
		{ 
			return; 
		}

		ImGuiWindowFlags flags = {};
		if (!ImGui::Begin("Scene Settings", &mDisplaySceneSettings, flags))
		{
			ImGui::End();
		}

		else
		{
			ImGuiTreeNodeFlags flags = {};

			// skybox
			if (ImGui::TreeNodeEx("Skybox Configurations", flags))
			{
				ImGui::Text(ICON_FA_QUESTION_CIRCLE_O " Drag and drop from Explorer and then:");
				ImGui::SameLine();

				if (ImGui::SmallButton("Apply"))
				{
					// rebuilds the skybox, possibly with new faces
				}

				const char* sides[6] = { "Right", "Left", "Top", "Bottom", "Front", "Back" };
				for (uint8_t i = 0; i < 6; i++)
				{
					//if (UI::ImageBrowser(sides[i], mSkyboxImages[i].descriptor, ImVec2(64.0f, 64.0f)))
					//{
					//	// attach new texture for right image
					//}
					//
					//if (i != 5) ImGui::SameLine();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}
}