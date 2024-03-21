# SoundTest

> This is basically what the core of the program is:
> ```cpp
> ioctl(fd, KIOCSOUND, static_cast<int>(CLOCK_RATE / frequency));
> std::this_thread::sleep_for(std::chrono::milliseconds(duration));
> ioctl(fd, KIOCSOUND, 0);
> ```
> Where `fd` is the file descriptor of the audio device, `CLOCK_RATE` is the clock rate of the PC
> speaker, `frequency` is the frequency of the sound, and `duration` is the duration of the sound.
>
> The `KIOCSOUND` constant is set in `linux/kd.h`.

## Getting Started

### Requirements

- A C++20 compiler ([GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), etc.)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [mpg123](https://www.mpg123.de/download.shtml)
- [libcurl](https://curl.se/libcurl/)

### Usage

```bash
./run.sh
```

## Notes

- For SoundCloud importing, you need an OAuth token from SoundCloud. You can get
  one [here](https://soundcloud.com/you/apps). Set the `SOUNDCLOUD_API_KEY` environment variable to that OAuth token.
- A workaround is to play a track on SoundCloud in your browser, then open developer tools and copy the value of
  the `oauth_token` cookie. This is your OAuth token as well.

## License

> [MIT](https://opensource.org/licenses/MIT)
