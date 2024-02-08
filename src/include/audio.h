#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>

#include "utils.h"

class AudioImporter
{
public:
    static void importWAV(std::vector<std::pair<int, int>> &data, const char* path);
    static void importMIDI(std::vector<std::pair<int, int>> &data, const char* path);
    static void importMP3(std::vector<std::pair<int, int>> &data, const char* path);
    static void importOGG(std::vector<std::pair<int, int>> &data, const char* path);
};
