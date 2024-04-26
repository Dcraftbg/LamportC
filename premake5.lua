workspace "Lamport"
        configurations {"Debug", "Release", "Dist"}
        architecture "x64"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
project "BCon_sha256"
        location "%{prj.name}"
        kind "StaticLib"
        language "C"
        targetdir ("out/bin/" .. outputdir .. "/%{prj.name}")
        objdir    ("out/int/" .. outputdir .. "/%{prj.name}")
        files {
           "%{prj.name}/include/**.h",
           "%{prj.name}/src/**.c",
        }
        includedirs {
           "%{prj.name}/include",
        }
macro_prefix = "LP"
project "Lamport"
        location "%{prj.name}"
        kind "ConsoleApp"
        language "C"
        targetdir ("out/bin/" .. outputdir .. "/%{prj.name}")
        objdir    ("out/int/" .. outputdir .. "/%{prj.name}")
        files {
           "%{prj.name}/src/**.h",
           "%{prj.name}/src/**.c",
        }
        libdirs {
           "out/bin/" .. outputdir .. "/BCon_sha256"
        }
        links {
           "BCon_sha256"
        }
        dependson { "BCon_sha256" }
        includedirs { "BCon_sha256/include" }
        filter "configurations:Debug"
            defines {
                macro_prefix .. "_TARGET_DEBUG"
            }
            symbols "On"
            optimize "Off"
        filter "configurations:Release"
            defines {
                macro_prefix .. "_TARGET_RELEASE"
            }
            symbols "On"
            optimize "On"
        filter "configurations:Dist"
            defines {
                macro_prefix .. "_TARGET_DIST"
            }
            symbols "Off"
            optimize "On"
        filter "system:Windows"
            defines {
                macro_prefix .. "_PLATFORM_WINDOWS"
            }
            links {
                "winmm"
            }
        filter "system:Linux"
            defines {
                macro_prefix .. "_PLATFORM_LINUX"
            }
        
        filter "system:Android"
            defines {
                macro_prefix .. "_PLATFORM_ANDROID"
            }
