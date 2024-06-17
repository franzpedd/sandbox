-- windows premake5 script
project "Editor"
    location "%{wks.location}/Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir(dir)
    objdir(obj)

    files
    {
        "%{includes.Editor}/**.h",
        "%{includes.Editor}/**.cpp"
    }
    
    includedirs
    {
        "%{includes.Editor}",
        "%{includes.Engine}",
        "%{includes.Vulkan}",
        "%{includes.Volk}",
        "%{includes.VMA}",
        "%{includes.GLM}",
        "%{includes.ImGui}",
        "%{includes.ImGuizmo}",
        "%{includes.Entt}",
    }

    links
    {
        "ImGui",
        "Engine"
    }
    
    filter "configurations:Debug"
        defines { "EDITOR_DEBUG" }
        runtime "Debug"
        symbols "On"

        links
        {

        }

        defines
        {
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "configurations:Release"
        defines { "EDITOR_RELEASE" }
        runtime "Release"
        optimize "On"

        links
        {

        }