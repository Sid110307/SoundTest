#include "include/audio.h"

void AudioImporter::importWAV(std::vector<std::pair<int, int>> &data, const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        error("Failed to open file: " + std::string(strerror(errno)));
        return;
    }

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    std::string header(buffer.begin(), buffer.begin() + 4);
    if (header != "RIFF")
    {
        error("Invalid WAV file! Missing RIFF header.");
        return;
    }

    auto chunkSize = *reinterpret_cast<int*>(&buffer[4]);
    auto sampleRate = *reinterpret_cast<int*>(&buffer[24]);
    auto bitsPerSample = *reinterpret_cast<short*>(&buffer[34]);
    auto numSamples = chunkSize / (bitsPerSample / 8);
    auto sampleDuration = 1.0 / sampleRate;

    if (bitsPerSample != 8 && bitsPerSample != 16)
    {
        error("Unsupported WAV file format! Only 8-bit and 16-bit WAV files are supported. Found " +
              std::to_string(bitsPerSample) + "-bit WAV file.");
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

    std::cout << "Imported " << data.size() << " notes from " << numSamples << " samples" << std::endl;
}

void AudioImporter::importMIDI(std::vector<std::pair<int, int>> &data, const char* path)
{
    auto processMIDITrack = [](const std::vector<unsigned char> &trackData, std::vector<std::pair<int, int>> &data)
    {
        size_t i = 0;
        while (i < trackData.size())
        {
            int deltaTime = 0;
            while (true)
            {
                deltaTime = deltaTime << 7 | (trackData[i] & 0x7F);
                if (trackData[i] & 0x80) ++i;
                else
                {
                    ++i;
                    break;
                }
            }

            int freq = 0;
            if (trackData[i] == 0xFF)
            {
                ++i;
                if (trackData[i] == 0x51)
                {
                    ++i;
                    int length = trackData[i];
                    ++i;
                    int ms = 0;

                    for (int j = 0; j < length; ++j)
                    {
                        ms = ms << 8 | trackData[i];
                        ++i;
                    }

                    data.emplace_back(0, ms);
                } else
                {
                    int length = trackData[i];
                    ++i;

                    for (int j = 0; j < length; ++j)
                    {
                        freq = freq << 8 | trackData[i];
                        ++i;
                    }

                    data.emplace_back(freq, deltaTime);
                }
            } else
            {
                freq = trackData[i];
                ++i;
            }

            int time = 0;
            while (true)
            {
                time = time << 7 | (trackData[i] & 0x7F);
                if (trackData[i] & 0x80) ++i;
                else
                {
                    ++i;
                    break;
                }
            }

            data.emplace_back(freq, time);
        }
    };

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        error("Failed to open file: " + std::string(strerror(errno)));
        return;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string header(buffer.begin(), buffer.begin() + 4);
    if (header != "MThd")
    {
        error("Invalid MIDI file! Missing MThd header.");
        return;
    }

    auto format = *reinterpret_cast<unsigned short*>(&buffer[8]);
    format = (format & 0xFF00) >> 8 | (format & 0x00FF) << 8;

    auto numTracks = *reinterpret_cast<unsigned short*>(&buffer[10]);
    numTracks = (numTracks & 0xFF00) >> 8 | (numTracks & 0x00FF) << 8;

    if (format == 0)
    {
        std::string trackHeader(buffer.begin() + 14, buffer.begin() + 18);
        if (trackHeader != "MTrk")
        {
            error("Invalid MIDI file! Missing MTrk header. Found " + trackHeader + " header.");
            return;
        }

        auto trackLength = *reinterpret_cast<unsigned int*>(&buffer[18]);
        trackLength =
            (trackLength & 0xFF000000) >> 24 | (trackLength & 0x00FF0000) >> 8 | (trackLength & 0x0000FF00) << 8 |
            (trackLength & 0x000000FF) << 24;

        std::vector<unsigned char> trackData(buffer.begin() + 22, buffer.begin() + 22 + trackLength);
        processMIDITrack(trackData, data);
    } else if (format == 1)
    {
        unsigned int i = 14;
        for (auto j = 0; j < numTracks; ++j)
        {
            std::string trackHeader(buffer.begin() + i, buffer.begin() + i + 4);
            if (trackHeader != "MTrk")
            {
                error("Invalid MIDI file! Missing MTrk header. Found " + trackHeader + " header.");
                return;
            }

            auto trackLength = *reinterpret_cast<unsigned int*>(&buffer[i + 4]);
            trackLength =
                (trackLength & 0xFF000000) >> 24 | (trackLength & 0x00FF0000) >> 8 | (trackLength & 0x0000FF00) << 8 |
                (trackLength & 0x000000FF) << 24;

            std::vector<unsigned char> trackData(buffer.begin() + i + 8, buffer.begin() + i + 8 + trackLength);
            processMIDITrack(trackData, data);

            i += 8 + trackLength;
        }
    } else
    {
        error("Unsupported MIDI file format! Only format 0 and format 1 MIDI files are supported. Found format " +
              std::to_string(format) + " MIDI file.");
        return;
    }

    std::cout << "Imported " << data.size() << " notes from " << numTracks << " tracks" << std::endl;
}

void AudioImporter::importMP3(std::vector<std::pair<int, int>> &data, const char* path)
{
    error("MP3 import not yet implemented!");
}

void AudioImporter::importOGG(std::vector<std::pair<int, int>> &data, const char* path)
{
    error("OGG import not yet implemented!");
}
