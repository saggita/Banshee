project "Banshee"
    kind "StaticLib"
    location "../Banshee"
    links {"assimp"}
    files { "../Banshee/**.h", "../Banshee/**.cpp", "../Banshee/**.cl", "../Banshee/**.fsh", "../Banshee/**.vsh" } 
    includedirs{"../3rdParty/assimp/include", "../3rdParty/oiio/include"} 
    
    buildoptions "-std=c++11 -stdlib=libc++"

    
    if os.is("macosx") then
    	links {"OpenImageIO"}
		libdirs {"../3rdParty/assimp/lib/x64", "../3rdParty/oiio/lib/x64"}
    end
	
	if os.is("windows") then
		configuration {"x32", "Debug"}
			links {"OpenImageIOD"}
			libdirs {"../3rdParty/assimp/lib/x86", "../3rdParty/oiio/lib/x86" }
		configuration {"x64", "Debug"}
			links {"OpenImageIOD"}
			libdirs {"../3rdParty/assimp/lib/x64", "../3rdParty/oiio/lib/x64" }
		configuration {"x32", "Release"}
			links {"OpenImageIO"}
			libdirs {"../3rdParty/assimp/lib/x86", "../3rdParty/oiio/lib/x86" }
		configuration {"x64", "Release"}
			links {"OpenImageIO"}
			libdirs {"../3rdParty/assimp/lib/x64", "../3rdParty/oiio/lib/x64" }
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
    