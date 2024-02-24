#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <SDL2/SDL.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

#include "include/utils.h"
#include "include/audio.h"

std::mutex dataMutex;
std::vector<std::pair<int, int>> data;
bool isPlaying = false, isDragging = false;
int currentFreq = 0, draggedIndex = -1;

void beep(int fd, int frequency, int duration)
{
    if (frequency == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        return;
    }

    check(fd);
    check(ioctl(fd, KIOCSOUND, static_cast<int>(CLOCK_RATE / frequency)));
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    check(ioctl(fd, KIOCSOUND, 0));
}

void playSounds()
{
    std::lock_guard<std::mutex> lock(dataMutex);
    int fd = open("/dev/console", O_WRONLY);

    for (const auto &[freq, duration]: data)
    {
        currentFreq = freq;
        beep(fd, freq, duration);
    }

    close(fd);
}

SDL_Window* init()
{
    check(SDL_Init(SDL_INIT_VIDEO));

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

void addImportButton(const char* label, void (* callback)(std::vector<std::pair<int, int>> &, const char*))
{
    if (ImGui::Button(label)) ImGui::OpenPopup(label);
    if (ImGui::BeginPopup(label))
    {
        static char path[256];
        ImGui::InputText("File Path", path, sizeof(path));
        if (ImGui::Button("OK"))
        {
            callback(data, path);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void drawToneGenerator()
{
    int numOctaves = 8, keysPerOctave = 12, startingOctave = 4;
    double baseFrequency = 440.0, semitoneRatio = std::pow(2.0, 1.0 / 12.0);
    std::string pianoKeyLabels[] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};

    static int duration = 500;
    ImGui::SliderInt("Duration", &duration, 0, 1000);

    for (int octave = 0; octave < numOctaves; ++octave)
        for (int i = 0; i < keysPerOctave; ++i)
        {
            int keyIndex = octave * keysPerOctave + i;
            double frequency = baseFrequency * std::pow(semitoneRatio, keyIndex);

            ImGui::PushID(keyIndex);
            std::string keyLabel = std::string(pianoKeyLabels[i]) + std::to_string(octave + startingOctave);

            if (ImGui::Button(keyLabel.c_str(), ImVec2(35, 35)))
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                data.emplace_back(static_cast<int>(std::round(frequency)), duration);
            }

            ImGui::PopID();
            if ((i + 1) % 12 != 0) ImGui::SameLine();
        }
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
    if (ImGui::Button("Clear"))
    {
        data.clear();
        isPlaying = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Sound")) data.emplace_back(440, 500);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.43f, 0.43f, 0.50f, 0.50f), "|");
    ImGui::SameLine();
    addImportButton("Import WAV", AudioImporter::importWAV);
    ImGui::SameLine();
    addImportButton("Import MIDI", AudioImporter::importMIDI);
    ImGui::SameLine();
    addImportButton("Import MP3", AudioImporter::importMP3);

    ImGui::SeparatorText("Tone Generator");
    drawToneGenerator();

    for (auto i = 0; i < static_cast<int>(data.size()); ++i)
    {
        auto &[freq, duration] = data[i];

        ImGui::PushID(i);
        ImGui::PushItemWidth(static_cast<float>(WIDTH) / 3);

        ImGui::SliderInt("Frequency", &freq, 0, 1000);
        ImGui::SameLine();
        ImGui::SliderInt("Duration", &duration, 0, 1000);

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
            std::thread sounds(playSounds);
            sounds.join();

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
