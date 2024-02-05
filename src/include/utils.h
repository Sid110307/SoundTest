#pragma once

#include <iostream>
#include <imgui/imgui.h>

constexpr int CLOCK_RATE = 1193182;
constexpr int WAV_CHUNK_SIZE = 1024;
constexpr double WAV_THRESHOLD = 0.1;

static int WIDTH = 1366, HEIGHT = 768;
static std::string errorMessage;
static std::vector<std::pair<int, int>> data;

void error(const std::string &message)
{
    if (ImGui::GetCurrentContext())
    {
        errorMessage = message;
        ImGui::OpenPopup("Error");
    }

    std::cerr << message << std::endl;
}

void check(int result)
{
    if (result < 0)
    {
        error("Error: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
}
