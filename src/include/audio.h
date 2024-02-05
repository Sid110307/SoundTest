#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

#include "utils.h"

namespace Audio
{
    void importWAV(const char* path);
    void importMIDI(const char* path);
    void importMP3(const char* path);
    void importOGG(const char* path);
}
