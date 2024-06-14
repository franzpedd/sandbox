-- global specification
workspace "Solution"
architecture "x86_64"
configurations { "Debug", "Release" };
startproject "Editor"

---- paths for solution build
dir = "%{wks.location}/_build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
obj = "%{wks.location}/_build/temp/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vkpath = os.getenv("VULKAN_SDK");

---- projects and dependencies header directories
includes = {}
includes["Engine"]  = "%{wks.location}/Engine/Source"
includes["Editor"]  = "%{wks.location}/Editor/Source"
--
includes["Vulkan"]  = os.getenv("VULKAN_SDK") .. "/Include"
includes["Volk"]    = "%{wks.location}/Thirdparty/volk"
includes["VMA"]    = "%{wks.location}/Thirdparty/vma/include"
includes["GLM"]     = "%{wks.location}/Thirdparty/glm"
includes["ImGui"] = "%{wks.location}/Thirdparty/imgui"
includes["ImGuizmo"] = "%{wks.location}/Thirdparty/imguizmo"
includes["STB"] = "%{wks.location}/Thirdparty/stb"

libraries = {}
libraries["Shaderc"] = os.getenv("VULKAN_SDK") .. "/Lib/shaderc_shared.lib"

---- dependecy projects
group "Thirdparty"
    include "Thirdparty/glm.lua"
    include "Thirdparty/imgui.lua"
group ""
---- projects by os
if os.host() == "windows" then
    include "Scripts/Build/windows_engine.lua"
    include "Scripts/Build/windows_editor.lua"
end