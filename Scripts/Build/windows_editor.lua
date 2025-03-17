-- windows premake5 script
project "Editor"
    location "%{wks.location}/Editor"
    kind "ConsoleApp"
    staticruntime "On"
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
        "%{includes.Renderer}",
        "%{includes.Engine}",
        "%{includes.Vulkan}",
        "%{includes.Volk}",
        "%{includes.VMA}",
        "%{includes.GLM}",
        "%{includes.ImGui}",
        "%{includes.ImGuizmo}",
        "%{includes.Entt}",
        "%{includes.TinyGLTF}",

        "%{wks.location}/Thirdparty/jolt"   -- Physics library
    }

    links
    {
        "Renderer",
        "ImGui",
        "Engine"
    }

    defines
    { 
        "_NO_CRT_STDIO_INLINE", 
        "JPH_DEBUG_RENDERER"
    }
    
    filter "configurations:Debug"
        defines { "EDITOR_DEBUG" }
        runtime "Debug"
        symbols "On"

        links
        {
            -- jolt
            --"%{wks.location}/Thirdparty/jolt/Build/Debug/Debug/Jolt.lib"
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
            -- jolt
            --"%{wks.location}/Thirdparty/jolt/Build/Release/Release/Jolt.lib"
        }