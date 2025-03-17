-- windows premake5 script
project "Renderer"
    location "%{wks.location}/Renderer"
    kind "StaticLib"
    staticruntime "On"
    language "C++"
    cppdialect "C++17"

    targetdir(dir)
    objdir(dir)
    
    files
    {
        "%{includes.Renderer}/**.h",
        "%{includes.Renderer}/**.cpp"
    }

    includedirs
    {
        "%{includes.Renderer}",
        "%{includes.Vulkan}",
        "%{includes.Volk}",
        "%{includes.VMA}",
        "%{includes.GLM}",
        "%{includes.STB}",
        "%{includes.TinyGLTF}"
    }

    links
    {
        "%{libraries.Shaderc}"
    }

    defines
    {
        "COSMOS_RENDERER_VULKAN", -- compiles vulkan code
    }

    filter "configurations:Debug"
        defines { "RENDERER_DEBUG" }
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
        }

        links
        {
        }

        linkoptions
         {
        }

    filter "configurations:Release"
        defines { "RENDERER_RELEASE" }
        runtime "Release"
        optimize "On"

        includedirs
        {

        }

        prebuildcommands
        {
        }

        links
        {
        }