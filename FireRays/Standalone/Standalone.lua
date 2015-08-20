project "Standalone"
    kind "ConsoleApp"
	includedirs {"../Banshee", "." }
    links {"Banshee", "assimp"}
    files { "**.cpp", "**.h" }
    includedirs{"../3rdParty/assimp/include", "../3rdParty/oiio/include"} 
    
    if os.is("linux") then
	links {"glut", "GLEW", "GL", "pthread", "OpenImageIO", "embree"} 
    	buildoptions "-std=c++11"
    end

    if os.is("macosx") or os.is("linux") then
    	links {"OpenImageIO"}
		libdirs {"../3rdParty/assimp/lib/x64", "../3rdParty/oiio/lib/x64"}
    	libdirs {"../3rdParty/embree/lib"}
    end

    if os.is("macosx") then
        linkoptions{ "-framework OpenGL", "-framework GLUT" }
        buildoptions "-std=c++11 -stdlib=libc++"
        links {"OpenImageIO"}
    end

    if os.is("windows") then
        includedirs { "../3rdParty/glew/include", "../3rdParty/freeglut/include"}
		links {"freeglut", "glew"}

		configuration {"x32"}
			libdirs { "../3rdParty/glew/lib/x86", "../3rdParty/freeglut/lib/x86"}
		configuration {"x64"}
			libdirs { "../3rdParty/glew/lib/x64", "../3rdParty/freeglut/lib/x64"}
    
    	configuration {}
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

    if _ACTION == "vs2012" then
	defines{ "GTEST_HAS_TR1_TUPLE=0" }
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
