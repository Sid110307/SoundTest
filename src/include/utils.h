#pragma once

#include <iostream>
#include <bit>

#include <imgui/imgui.h>

constexpr int WIDTH = 1366;
constexpr int HEIGHT = 768;
constexpr int CLOCK_RATE = 1193182;
constexpr int CHUNK_SIZE = 1000;
constexpr double THRESHOLD = 0.1;

static std::string errorMessage;

static void error(const std::string &message)
{
    if (ImGui::GetCurrentContext())
    {
        errorMessage = message;
        ImGui::OpenPopup("Error");

        if (ImGui::BeginPopup("Error"))
        {
            ImGui::Text("%s", errorMessage.c_str());
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    std::cerr << message << std::endl;
}

template<typename T>
static T check(T result)
{
    if (result < 0)
    {
        error("Error: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    return result;
}
