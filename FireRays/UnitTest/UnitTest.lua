project "UnitTest"
    kind "ConsoleApp"
    includedirs {"../Banshee", "." }
    links {"Banshee", "assimp", "Gtest"}
    files { "**.cpp", "**.h" }
    includedirs{"../3rdParty/assimp/include", "../3rdParty/oiio/include", "../Gtest/include"} 
    
    if  os.is("linux") then
        buildoptions "-std=c++11"
        links {"OpenImageIO"}
    end

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
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
