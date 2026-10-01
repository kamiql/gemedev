#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <gemedev/Audio.hpp>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace gd {
/** Owns a miniaudio engine and its default playback device. */
struct AudioSystem::Impl {
    ma_engine engine{};
    /** Starts the audio engine or reports a device failure. */
    Impl() {
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS)
            throw std::runtime_error("Could not initialize the audio playback device");
    }
    /** Stops playback and releases the default audio device. */
    ~Impl() { ma_engine_uninit(&engine); }
};
/** Initializes the default sound backend. */
AudioSystem::AudioSystem() : impl_(std::make_unique<Impl>()) {}
/** Releases the backend and currently playing sounds. */
AudioSystem::~AudioSystem() = default;
/** Validates a file before creating a lightweight, reusable sound handle. */
SoundHandle AudioSystem::load(const std::string& path) const {
    if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Sound file not found: " + path);
    return {path};
}
/** Starts playback without blocking the game loop. */
void AudioSystem::play(const SoundHandle& sound) {
    if (sound.path.empty() || ma_engine_play_sound(&impl_->engine, sound.path.c_str(), nullptr) != MA_SUCCESS)
        throw std::runtime_error("Could not play sound: " + sound.path);
}
/** Sets the global sound volume. */
void AudioSystem::setVolume(float volume) { ma_engine_set_volume(&impl_->engine, std::clamp(volume, 0.0f, 1.0f)); }
}
