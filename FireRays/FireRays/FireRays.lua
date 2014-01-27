project "FireRays"
    kind "StaticLib"
    location "../FireRays"
    includedirs { "./Core", "./Util" }
    files { "../FireRays/**.h", "../FireRays/**.cpp", "../FireRays/**.cl", "../FireRays/**.fsh", "../FireRays/**.vsh" }
    buildoptions "-std=c++11 -stdlib=libc++"
	
	configuration {"x32", "Debug"}
        targetdir "../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
	
	if os.is("windows") then
		if _ACTION == "vs2010" then
		includedirs { "../3rdParty/assimp/include", "../3rdParty/glew/include", "../3rdParty/freeglut/include" }
		end
	end
    

		
