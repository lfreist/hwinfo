--
-- Main Premake5 file for building HwInfo project.
-- Copyright (c) 2023 by Danil (Kenny) Dukhovenko, All rights reserved.
--

-- HwInfo C++ Project
project "HwInfo"
	kind "StaticLib"
	language "C++"
	staticruntime "on"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files {
		"include/hwinfo/utils/filesystem.h",
		"include/hwinfo/utils/stringutils.h",
		"include/hwinfo/utils/subprocess.h",
		"include/hwinfo/battery.h",
		"include/hwinfo/cpu.h",
		"include/hwinfo/cpuid.h",
		"include/hwinfo/disk.h",
		"include/hwinfo/gpu.h",
		"include/hwinfo/hwinfo.h",
		"include/hwinfo/mainboard.h",
		"include/hwinfo/os.h",
		"include/hwinfo/platform.h",
		"include/hwinfo/ram.h",
		"include/hwinfo/system.h",
		"include/hwinfo/WNIwrapper.h",
		"src/battery.cpp",
		"src/cpu.cpp",
		"src/disk.cpp",
		"src/gpu.cpp",
		"src/mainboard.cpp",
		"src/os.cpp",
		"src/ram.cpp",
		"src/system.cpp"
	}

	filter "system:windows"
		systemversion "latest"

		files {
			"src/windows/utis/filesystem.cpp",
			"src/windows/battery.cpp",
			"src/windows/cpu.cpp",
			"src/windows/disk.cpp",
			"src/windows/gpu.cpp",
			"src/windows/mainboard.cpp",
			"src/windows/os.cpp",
			"src/windows/ram.cpp"
		}

		defines  { 
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "system:linux"
		files {
			"src/linux/utis/filesystem.cpp",
			"src/linux/battery.cpp",
			"src/linux/cpu.cpp",
			"src/linux/disk.cpp",
			"src/linux/gpu.cpp",
			"src/linux/mainboard.cpp",
			"src/linux/os.cpp",
			"src/linux/ram.cpp"
		}

	filter "system::macosx"
		files {
			"src/apple/utis/filesystem.cpp",
			"src/apple/battery.cpp",
			"src/apple/cpu.cpp",
			"src/apple/disk.cpp",
			"src/apple/gpu.cpp",
			"src/apple/mainboard.cpp",
			"src/apple/os.cpp",
			"src/apple/ram.cpp"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"