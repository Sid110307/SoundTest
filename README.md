# SoundTest

> This is basically what the core of the program is:
> ```cpp
> ioctl(fd, KIOCSOUND, static_cast<int>(CLOCK_RATE / frequency));
> std::this_thread::sleep_for(std::chrono::milliseconds(duration));
> ioctl(fd, KIOCSOUND, 0);
> ```
> Where `fd` is the file descriptor of the `/dev/console` file, `CLOCK_RATE` is the clock rate of the PC
> speaker, `frequency` is the frequency of the sound, and `duration` is the duration of the sound.
>
> The `KIOCSOUND` constant is set in `linux/kd.h`.

## Getting Started

### Requirements

* A C++20 compiler ([GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), etc.)
* [SDL2](https://www.libsdl.org/download-2.0.php)
* [mpg123](https://www.mpg123.de/download.shtml)

### Usage

```bash
./run.sh
```

## License

> [MIT](https://opensource.org/licenses/MIT)
