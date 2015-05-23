#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace snd {
        #define SOUND_CHANNEL_COUNT 8

        typedef enum {
            SAMPLE_PCM8,
            SAMPLE_PCM16
        } SampleFormat;

        void* salloc(u32 size);
        void sfree(void* ptr);

        bool play(u32 channel, void *samples, u32 numSamples, SampleFormat format, u32 sampleRate, float leftVolume, float rightVolume, bool loop);
        bool stop(u32 channel);
        bool flushCommands();
    }
}