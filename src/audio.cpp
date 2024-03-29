#include "include/audio.h"

bool AudioManager::skipHeader = false;

void AudioManager::importWAV(std::vector<std::pair<int, int>> &data, const char* path)
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

    for (auto i = 0; i < numSamples; i += CHUNK_SIZE)
    {
        std::vector<int> chunk(samples.begin() + i,
                               samples.begin() + std::min(i + CHUNK_SIZE, numSamples));

        auto max = *std::max_element(chunk.begin(), chunk.end());
        auto min = *std::min_element(chunk.begin(), chunk.end());
        auto amplitude = max - min;

        if (amplitude > THRESHOLD)
            data.emplace_back(sampleRate / amplitude,
                              static_cast<int>(sampleDuration * static_cast<int>(chunk.size()) * 1000));
    }

    std::cout << "Imported " << data.size() << " notes from " << numSamples << " samples" << std::endl;
}

void AudioManager::importMIDI(std::vector<std::pair<int, int>> &data, const char* path)
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

void AudioManager::importMP3(std::vector<std::pair<int, int>> &data, const char* path)
{
    mpg123_init();
    int err = 0;

    auto mh = mpg123_new(nullptr, &err);
    if (mh == nullptr)
    {
        error("Failed to create mpg123 handle: " + std::string(mpg123_plain_strerror(err)));
        return;
    }
    if (err != MPG123_OK)
    {
        error("Failed to initialize mpg123: " + std::string(mpg123_plain_strerror(err)));
        return;
    }

    check(mpg123_open(mh, path));
    long rate = 0;
    int channels = 0, encoding = 0;

    mpg123_getformat(mh, &rate, &channels, &encoding);
    if (encoding != MPG123_ENC_SIGNED_16)
    {
        error("Unsupported MP3 file format! Only 16-bit MP3 files are supported. Found " +
              std::to_string(mpg123_encsize(encoding)) + "-bit MP3 file.");
        return;
    }

    auto bufferSize = mpg123_outblock(mh);
    auto buffer = new unsigned char[bufferSize];
    size_t done = 0;
    size_t samples = 0;

    do
    {
        err = mpg123_read(mh, buffer, bufferSize, &done);
        for (size_t i = 0; i < done; i += 2) samples += buffer[i] << 8 | buffer[i + 1];
    } while (err == MPG123_OK);

    if (err != MPG123_DONE)
    {
        error("Failed to read MP3 file: " + std::string(mpg123_strerror(mh)));
        return;
    }

    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    auto sampleDuration = 1.0 / static_cast<double>(rate);
    for (size_t i = 0; i < samples; i += CHUNK_SIZE)
    {
        std::vector<int> chunk(buffer + i, buffer + std::min(i + CHUNK_SIZE, samples));
        auto max = *std::max_element(chunk.begin(), chunk.end());
        auto min = *std::min_element(chunk.begin(), chunk.end());
        auto amplitude = max - min;

        if (amplitude > THRESHOLD)
            data.emplace_back(rate / amplitude,
                              static_cast<int>(sampleDuration * static_cast<int>(chunk.size()) * 1000));
    }

    delete[] buffer;
    std::cout << "Imported " << data.size() << " notes from " << samples << " samples" << std::endl;
}

void AudioManager::importCSV(std::vector<std::pair<int, int>> &data, const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        error("Failed to open file: " + std::string(strerror(errno)));
        return;
    }

    std::string line;
    if (skipHeader) std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string freq, duration;

        std::getline(ss, freq, ',');
        std::getline(ss, duration, ',');

        if (freq.empty() || duration.empty()) continue;

        try { data.emplace_back(std::stoi(freq), std::stoi(duration)); }
        catch (std::invalid_argument &e) { error("Invalid data in CSV file: " + std::string(freq) + ", " + duration); }
    }

    std::cout << "Imported " << data.size() << " notes from " << data.size() << " samples" << std::endl;
}

void AudioManager::importSoundCloud(std::vector<std::pair<int, int>> &data, const char* id)
{
    std::string filename = "soundcloud_" + std::string(id) + ".mp3";
    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        error("Failed to initialize cURL");
        return;
    }

    std::string url = "http://api.soundcloud.com/tracks/" + std::string(id) + "/download";
    std::string buffer;

    struct curl_slist* headers = nullptr;
    curl_slist_append(headers, ("Authorization: OAuth " + std::string(SOUNDCLOUD_API_KEY)).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* ptr, size_t size, size_t nmemb, void* stream) -> size_t
    {
        auto data = reinterpret_cast<std::string*>(stream);
        data->append(reinterpret_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    check(curl_easy_perform(curl));
    curl_easy_cleanup(curl);

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        error("Failed to open file: " + std::string(strerror(errno)));
        return;
    }

    file.write(buffer.c_str(), static_cast<long>(buffer.size()));
    file.close();

    importMP3(data, filename.c_str());
    if (file.good()) remove(filename.c_str());

    std::cout << "Imported " << data.size() << " notes from SoundCloud track " << id << std::endl;
}

void AudioManager::exportCSV(std::vector<std::pair<int, int>> &data, const char* path)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        error("Failed to open file: " + std::string(strerror(errno)));
        return;
    }

    file << "Frequency (Hz),Duration (ms)" << std::endl;
    for (auto &note: data) file << note.first << ',' << note.second << std::endl;
    file.close();

    std::cout << "Exported " << data.size() << " notes to " << path << std::endl;
}
