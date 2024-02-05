#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <SDL2/SDL.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

constexpr int CLOCK_RATE = 1193182;
constexpr int WAV_CHUNK_SIZE = 1024;
constexpr double WAV_THRESHOLD = 0.1;

int WIDTH = 1366, HEIGHT = 768;
std::vector<std::pair<int, int>> data = {{440, 500},
                                         {220, 500}};
bool isPlaying = false, isDragging = false;
std::string errorMessage;
int currentFreq = 0, draggedIndex = -1;

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

void beep(int fd, int frequency, int length)
{
    check(fd);

    check(ioctl(fd, KIOCSOUND, static_cast<int>(CLOCK_RATE / frequency)));
    std::this_thread::sleep_for(std::chrono::milliseconds(length));
    check(ioctl(fd, KIOCSOUND, 0));
}

void importWAV(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) error("Failed to open file: " + std::string(strerror(errno)));
    else
    {
        file.seekg(0, std::ios::end);
        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);
        file.close();

        std::string header(buffer.begin(), buffer.begin() + 4);
        if (header != "RIFF") error("Invalid WAV file!");
        else
        {
            auto chunkSize = *reinterpret_cast<int*>(&buffer[4]);
            auto sampleRate = *reinterpret_cast<int*>(&buffer[24]);
            auto bitsPerSample = *reinterpret_cast<short*>(&buffer[34]);
            auto numSamples = chunkSize / (bitsPerSample / 8);
            auto sampleDuration = 1.0 / sampleRate;

            if (bitsPerSample != 16)
            {
                error("Only 16-bit WAV files are supported! This file is " + std::to_string(bitsPerSample) + "-bit.");
                return;
            }

            std::vector<int> samples(numSamples);
            for (auto i = 0; i < numSamples; ++i) samples[i] = *reinterpret_cast<short*>(&buffer[44 + i * 2]);

            for (auto i = 0; i < numSamples; i += WAV_CHUNK_SIZE)
            {
                std::vector<int> chunk(samples.begin() + i, samples.begin() + std::min(i + WAV_CHUNK_SIZE, numSamples));

                auto max = *std::max_element(chunk.begin(), chunk.end());
                auto min = *std::min_element(chunk.begin(), chunk.end());
                auto amplitude = max - min;

                if (amplitude > WAV_THRESHOLD)
                    data.emplace_back(sampleRate / amplitude,
                                      static_cast<int>(sampleDuration * static_cast<int>(chunk.size()) * 1000));
            }

            std::cout << "Imported " << numSamples << " samples at " << sampleRate << " Hz" << std::endl;
        }
    }
}

SDL_Window* init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        error("Failed to initialize SDL! Error: " + std::string(SDL_GetError()));
        exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window* window = SDL_CreateWindow("SoundTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        error("Failed to create window! Error: " + std::string(SDL_GetError()));
        SDL_Quit();

        exit(EXIT_FAILURE);
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        error("Failed to create OpenGL context! Error: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(window);
        SDL_Quit();

        exit(EXIT_FAILURE);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    return window;
}

void drawGUI(SDL_Window* window)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    ImGui::Begin("SoundTest", nullptr, ImGuiWindowFlags_NoTitleBar);
    ImGui::SeparatorText("Sound Data");

    if (ImGui::Button("Play")) isPlaying = true;
    ImGui::SameLine();
    if (ImGui::Button("Stop")) isPlaying = false;
    ImGui::SameLine();
    if (ImGui::Button("Add Sound")) data.emplace_back(440, 500);
    ImGui::SameLine();
    if (ImGui::Button("Import WAV")) ImGui::OpenPopup("Import WAV");

    for (auto i = 0; i < static_cast<int>(data.size()); ++i)
    {
        auto &[freq, length] = data[i];

        ImGui::PushID(i);
        ImGui::PushItemWidth(static_cast<float>(WIDTH) / 3);

        ImGui::SliderInt("Frequency", &freq, 0, 1000);
        ImGui::SameLine();
        ImGui::SliderInt("Length", &length, 0, 1000);

        ImGui::SameLine();
        ImGui::Text(" ");
        if (i > 0)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("^##up")) std::swap(data[i], data[i - 1]);
        }

        if (i < static_cast<int>(data.size()) - 1)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("v##down")) std::swap(data[i], data[i + 1]);
        }

        ImGui::SameLine();
        if (ImGui::Button("Delete")) data.erase(data.begin() + i);

        if (ImGui::IsMouseReleased(0) && isDragging && draggedIndex != -1)
        {
            isDragging = false;
            if (draggedIndex != i)
                std::rotate(data.begin() + std::min(i, draggedIndex), data.begin() + draggedIndex,
                            data.begin() + std::max(i, draggedIndex) + 1);
        }

        if (ImGui::IsItemActive() && !isDragging)
        {
            draggedIndex = i;
            isDragging = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
    }

    ImGui::SeparatorText("Audio Device");

    static char audioDevice[256] = "/dev/console";
    ImGui::InputText("Audio Device", audioDevice, sizeof(audioDevice));
    ImGui::Text("Currently Playing: %d Hz", currentFreq);

    static char path[256] = "";
    if (ImGui::BeginPopup("Import WAV"))
    {
        ImGui::InputText("File Path", path, sizeof(path));
        if (ImGui::Button("OK"))
        {
            importWAV(path);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Error"))
    {
        ImGui::Text("%s", errorMessage.c_str());
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    if (geteuid() != 0)
    {
        error("Please run this program as root!");
        return EXIT_FAILURE;
    }

    SDL_Window* window = init();
    bool running = true;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        drawGUI(window);
        if (isPlaying)
        {
            int fd = open("/dev/console", O_WRONLY);
            for (const auto &[freq, length]: data)
            {
                currentFreq = freq;
                beep(fd, freq, length);
            }

            close(fd);

            currentFreq = 0;
            isPlaying = false;
        }

        SDL_GL_SwapWindow(window);
        SDL_RenderPresent(SDL_GetRenderer(window));
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
