project "Experiments"
    kind "ConsoleApp"
    location "../Experiments"
    links {"FireRays", "CLW", "assimp"}
    files { "../Experiments/**.h", "../Experiments/**.cpp"} 
    includedirs{ "../FireRays/Core", "../FireRays/Util", "../3rdParty/assimp/include", "../CLW" } 
    
    buildoptions "-std=c++11 -stdlib=libc++"

    
    if os.is("macosx") then
		libdirs {"../3rdParty/assimp/lib/x64"}
    end
	
    if os.is("windows") then
	links {"FireRays", "assimp", "glew", "freeglut"}
        includedirs {"../3rdParty/glew/include", "../3rdParty/freeglut/include"}
		configuration {"x32"}
			libdirs {"../3rdParty/assimp/lib/x86", "../3rdParty/glew/lib/x86", "../3rdParty/freeglut/lib/x86"}
		configuration {"x64"}
			libdirs {"../3rdParty/assimp/lib/x64", "../3rdParty/glew/lib/x64", "../3rdParty/freeglut/lib/x86"}
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
    