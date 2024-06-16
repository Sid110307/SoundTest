#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <algorithm>

#include <mpg123.h>
#include <curl/curl.h>

#include "utils.h"

class AudioManager
{
public:
    static void importWAV(std::vector<std::pair<int, int>> &data, const char* path);
    static void importMIDI(std::vector<std::pair<int, int>> &data, const char* path);
    static void importMP3(std::vector<std::pair<int, int>> &data, const char* path);
    static void importCSV(std::vector<std::pair<int, int>> &data, const char* path);
    static void importSoundCloud(std::vector<std::pair<int, int>> &data, const char* id);
    static void exportCSV(std::vector<std::pair<int, int>> &data, const char* path);

    static bool skipHeader;
};
