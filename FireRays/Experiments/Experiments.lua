project "Experiments"
    kind "ConsoleApp"
    location "../Experiments"
    links {"FireRays", "assimp"}
    files { "../Experiments/**.h", "../Experiments/**.cpp"} 
    includedirs{ "../FireRays/Core", "../FireRays/Util", "../3rdParty/assimp/include" } 
    
    buildoptions "-std=c++11 -stdlib=libc++"

    
    if os.is("macosx") then
		libdirs {"../3rdParty/assimp/lib/x64"}
    end
	
	if os.is("windows") then
		links {"assimp"}
		configuration {"x32"}
			libdirs {"../3rdParty/assimp/lib/x86"}
		configuration {"x64"}
			libdirs {"../3rdParty/assimp/lib/x64"}
    end
	
    configuration {"x32", "Debug"}
        targetdir "../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
    