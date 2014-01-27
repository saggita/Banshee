function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

solution "FireRays"
	configurations { "Debug", "Release" }    		
	language "C++"
	flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }
    -- find and add path to Opencl headers
	dofile ("./OpenCLSearch.lua" )
    -- define common includes
    includedirs { ".","./3rdParty/include" }

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
        if _ACTION == "vs2010" then
            buildoptions { "/MP"  } --multiprocessor build
            defines {"_CRT_SECURE_NO_WARNINGS"}
            configuration {"Release"}
        end
		--os.execute("mkdir dist\\debug\\bin\\")
		--os.execute("cp -R ./contrib/bin/* ./dist/debug/bin/")
		--os.execute("mkdir dist\\release\\bin\\")
		--os.execute("cp -R ./contrib/bin/* ./dist/release/bin/")
	end

	--make configuration specific definitions
    configuration "Debug"
		defines { "_DEBUG" }
		flags { "Symbols" }
        --includedirs { "./dist/debug/include" }
	configuration "Release"
		defines { "NDEBUG" }
        --includedirs { "./dist/release/include" }

	configuration {"x64", "Debug"}
		targetsuffix "64D"
        --libdirs { "./dist/debug/lib/x86_64" }
        --targetdir "./dist/debug/bin/x86_64"
	configuration {"x32", "Debug"}
		targetsuffix "D"
        --libdirs { "./dist/debug/lib/x86" }
        --targetdir "./dist/debug/bin/x86"
	configuration {"x64", "Release"}
		targetsuffix "64"
        --libdirs { "./dist/release/lib/x86_64" }
        --targetdir "./dist/release/bin/x86_64"
	--configuration {"x32", "Release"}
        --libdirs { "./dist/release/lib/x86" }
        --targetdir "./dist/release/bin/x86"
		--flags { "Optimize" }

	configuration "x64"
		--libdirs { "./contrib/lib/"..targetName.."64" }
		--defines {"_X64"}
	configuration "x32"
		--libdirs { "./contrib/lib/"..targetName }
    
    configuration {} -- back to all configurations
    -- generate the projects

    if fileExists("./FireRays/FireRays.lua") then
		dofile("./FireRays/FireRays.lua")
	end
	
	if fileExists("./Launcher/Launcher.lua") then
		dofile("./Launcher/Launcher.lua")
	end
