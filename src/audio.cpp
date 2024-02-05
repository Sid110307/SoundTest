#include "include/audio.h"

using namespace Audio;

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

            if (bitsPerSample != 8 && bitsPerSample != 16)
            {
                error("Unsupported WAV file format! Only 8-bit and 16-bit WAV files are supported.");
                return;
            }

            std::vector<int> samples(numSamples);
            for (auto i = 0; i < numSamples; ++i)
                samples[i] = bitsPerSample == 8 ? static_cast<int>(static_cast<unsigned char>(buffer[44 + i])) - 128
                                                : *reinterpret_cast<short*>(&buffer[44 + i * 2]);

            for (auto i = 0; i < numSamples; i += WAV_CHUNK_SIZE)
            {
                std::vector<int> chunk(samples.begin() + i,
                                       samples.begin() + std::min(i + WAV_CHUNK_SIZE, numSamples));

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

void importMIDI(const char* path)
{
    error("MIDI import not yet implemented!");
}

void importMP3(const char* path)
{
    error("MP3 import not yet implemented!");
}

void importOGG(const char* path)
{
    error("OGG import not yet implemented!");
}
