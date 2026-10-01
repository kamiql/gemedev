#pragma once
#include <memory>
#include <string>

namespace gd {
/** A reusable reference to an audio file supported by the audio backend. */
struct SoundHandle {
    std::string path;
};
/** Manages an audio device and supports fire-and-forget sound playback. */
class AudioSystem {
public:
    /** Opens the default playback device or throws on initialization failure. */
    AudioSystem();
    /** Closes the audio device after outstanding playbacks end. */
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    /** Validates an audio path and returns a reusable path-based sound handle. */
    SoundHandle load(const std::string& path) const;
    /** Plays a sound asynchronously and throws when playback cannot start. */
    void play(const SoundHandle& sound);
    /** Changes the master volume, clamped to the range 0 to 1. */
    void setVolume(float volume);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
