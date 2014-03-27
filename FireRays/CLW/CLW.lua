project "CLW"
    kind "StaticLib"
    location "../CLW"
    includedirs { "." }
    files { "../CLW/**.h", "../CLW/**.cpp", "../CLW/**.cl" }
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
    

		
