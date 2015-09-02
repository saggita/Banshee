function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption
{
	trigger = "use_embree",
	description = "Use Intel(R) Embree intersection accelerator"
}

newoption
{
	trigger = "use_oiio16",
	description = "Use OpeImageIO 1.6 version, default one is 1.5"
}

solution "Banshee"
	configurations { "Debug", "Release" }    		
	language "C++"
	flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }

    -- perform OS specific initializations
    local targetName;
    if os.is("macosx") then
        targetName = "osx"
		platforms {"x64"}
	else
		platforms {"x32", "x64"}
	end
	
    if os.is("windows") then
        targetName = "win"
		defines{ "WIN32" }
        if _ACTION == "vs2010" or _ACTION == "vs2012" or _ACTION == "vs2013" then
            buildoptions { "/MP"  } --multiprocessor build
            defines {"_CRT_SECURE_NO_WARNINGS"}
        end
	end

	if _OPTIONS["use_embree"] then 
			defines {"USE_EMBREE"}
	end

	if _OPTIONS["use_oiio16"] then
			defines {"USE_OIIO16"}
	end

	--make configuration specific definitions
    configuration "Debug"
		defines { "_DEBUG" }
		flags { "Symbols" }
        --includedirs { "./dist/debug/include" }
	configuration "Release"
		defines { "NDEBUG" }
		flags {"Optimize"}
        --includedirs { "./dist/release/include" }

	configuration {"x64", "Debug"}
		targetsuffix "64D"
	configuration {"x32", "Debug"}
		targetsuffix "D"
	configuration {"x64", "Release"}
		targetsuffix "64"
    
    configuration {} -- back to all configurations
	
	if fileExists("./Banshee/Banshee.lua") then
		dofile("./Banshee/Banshee.lua")
	end

	if fileExists("./Standalone/Standalone.lua") then
		dofile("./Standalone/Standalone.lua")
	end

	if fileExists("./Gtest/gtest.lua") then
		dofile("./Gtest/gtest.lua")
	end

	if fileExists("./UnitTest/UnitTest.lua") then
		dofile("./UnitTest/UnitTest.lua")
	end
	
