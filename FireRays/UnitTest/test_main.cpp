#include "basic_features.h"
#include "materials.h"
#include "internals.h"

#include <gtest/gtest.h>

#include <string>
#include <algorithm>
#include <iostream>
#include <string>

std::string g_output_image_path = "./OutputImages";
std::string g_texture_path = "../../../Resources/Textures";
std::string g_ref_image_path = "../../../Resources/Reference";
bool g_compare = false;
int2 g_imgres = int2(256, 256);
int  g_num_spp = 4;


char* GetCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool CmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    // Handle command line parameters
    char* output_image_path = GetCmdOption(argv, argv + argc, "--output_path");
    g_output_image_path = output_image_path ? output_image_path : g_output_image_path;
    
    char* texture_path = GetCmdOption(argv, argv + argc, "--texture_path");
    g_texture_path = texture_path ? texture_path : g_texture_path;
    
    char* reference_image_path = GetCmdOption(argv, argv + argc, "--reference_path");
    g_ref_image_path = reference_image_path ? reference_image_path : g_ref_image_path;
    
    char* spp = GetCmdOption(argv, argv + argc, "--spp");
    g_num_spp = spp ? atoi(spp) : g_num_spp;
    
    g_compare = CmdOptionExists(argv, argv + argc, "--compare") ? true : false;
    
    return RUN_ALL_TESTS();
}
