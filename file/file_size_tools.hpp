//
// Created by Erik Sevre on 11/6/2019.
//

#pragma once

#include <fstream>
#include <string>

long long get_file_size(const std::string &filename)
{
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    return infile.tellg();
}


