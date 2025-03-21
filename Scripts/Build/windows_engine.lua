-- windows premake5 script
project "Engine"
    location "%{wks.location}/Engine"
    kind "StaticLib"
    staticruntime "On"
    language "C++"
    cppdialect "C++17"

    pchheader "epch.h"
    pchsource "%{includes.Engine}/epch.cpp"

    targetdir(dir)
    objdir(dir)
    
    files
    {
        "%{includes.Engine}/**.h",
        "%{includes.Engine}/**.cpp"
    }

    includedirs
    {
        "%{includes.Engine}",
        "%{includes.Renderer}",
        "%{includes.Vulkan}",
        "%{includes.Volk}",
        "%{includes.VMA}",
        "%{includes.GLM}",
        "%{includes.ImGui}",
        "%{includes.ImGuizmo}",
        "%{includes.STB}",
        "%{includes.Entt}",
        "%{includes.TinyGLTF}",

        "%{wks.location}/Thirdparty/sdl/SDL2-2.30.2/include",   -- SDL location changes depending on operating system
        "%{wks.location}/Thirdparty/imgui/backends",            -- ImGui rely on backend implementation, we're using SDL2+Vulkan
        "%{wks.location}/Thirdparty/jolt"                       -- Physics library
    }

    links
    {
        "Renderer",
        "ImGui",
        "%{libraries.Shaderc}"
    }

    defines
    {
        "COSMOS_RENDERER_VULKAN", -- compiles vulkan code
        "_NO_CRT_STDIO_INLINE", 
        "JPH_DEBUG_RENDERER"
    }

    filter "configurations:Debug"
        defines { "ENGINE_DEBUG" }
        runtime "Debug"
        symbols "On"

        includedirs
        {
        
        }

        defines
        {
            "_CRT_SECURE_NO_WARNINGS"
        }

        prebuildcommands
        {
            "{COPYFILE} %{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2.dll " .. dir;
        }

        links
        {
            -- sdl
            "%{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2.lib",
            "%{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2main.lib",
            -- jolt
            "%{wks.location}/Thirdparty/jolt/Build/Debug/Debug/Jolt.lib"
        }

        linkoptions
        {
            "/ignore:4006"
        }

    filter "configurations:Release"
        defines { "ENGINE_RELEASE" }
        runtime "Release"
        optimize "On"

        includedirs
        {

        }

        prebuildcommands
        {
            "{COPYFILE} %{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2.dll " .. dir;
        }

        links
        {
            -- sdl
            "%{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2.lib",
            "%{wks.location}/Thirdparty/sdl/SDL2-2.30.2/lib/x64/SDL2main.lib",
            -- jolt
            "%{wks.location}/Thirdparty/jolt/Build/Release/Release/Jolt.lib"
        }