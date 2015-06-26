#include "citrus/snd.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <3ds.h>

namespace ctr {
    namespace snd {
        static bool initialized = false;
    }
}

bool ctr::snd::init() {
    ctr::err::parse((u32) csndInit());
    initialized = !ctr::err::has();
    return initialized;
}

void ctr::snd::exit() {
    if(!initialized) {
        return;
    }

    initialized = false;
    csndExit();
}

void* ctr::snd::salloc(u32 size) {
    if(!initialized) {
        return NULL;
    }

    return linearAlloc(size);
}

void ctr::snd::sfree(void* mem) {
    if(!initialized) {
        return;
    }

    linearFree(mem);
}

bool ctr::snd::play(u32 channel, void *samples, u32 numSamples, ctr::snd::SampleFormat format, u32 sampleRate, float leftVolume, float rightVolume, bool loop) {
    if(!initialized || samples == NULL) {
        return false;
    }

    u32 chn = 8 + channel;
    u32 flags = (format == SAMPLE_PCM16 ? SOUND_FORMAT_16BIT : SOUND_FORMAT_8BIT) | (loop ? SOUND_REPEAT : SOUND_ONE_SHOT);
    u32 size = numSamples * (format == SAMPLE_PCM16 ? 2 : 1);

    if(!(csndChannels & BIT(chn))) {
        return false;
    }

    if(leftVolume < 0) {
        leftVolume = 0;
    } else if(leftVolume > 1) {
        leftVolume = 1;
    }

    if(rightVolume < 0) {
        rightVolume = 0;
    } else if(rightVolume > 1) {
        rightVolume = 1;
    }

    GSPGPU_FlushDataCache(NULL, (u8*) samples, size);

    u32 paddr0 = 0;
    u32 paddr1 = 0;
    u32 encoding = (flags >> 12) & 3;
    if(encoding != CSND_ENCODING_PSG) {
        paddr0 = osConvertVirtToPhys((u32) samples);
        paddr1 = osConvertVirtToPhys((u32) samples);
        if(encoding == CSND_ENCODING_ADPCM) {
            int adpcmSample = ((s16*) samples)[-2];
            int adpcmIndex = ((u8*) samples)[-2];
            CSND_SetAdpcmState(chn, 0, adpcmSample, adpcmIndex);
        }
    }

    u32 timer = CSND_TIMER(sampleRate);
    if(timer < 0x0042) {
        timer = 0x0042;
    } else if(timer > 0xFFFF) {
        timer = 0xFFFF;
    }

    flags &= ~0xFFFF001F;
    flags |= SOUND_ENABLE | SOUND_CHANNEL(chn) | (timer << 16);

    u32 volumes = (u32) (leftVolume * 0x8000) | ((u32) (rightVolume * 0x8000) << 16);
    CSND_SetChnRegs(flags, paddr0, paddr1, size, volumes, volumes);

    u32 loopMode = (flags >> 10) & 3;
    if(loopMode == CSND_LOOPMODE_NORMAL && paddr1 > paddr0) {
        size -= paddr1 - paddr0;
        CSND_SetBlock(chn, 1, paddr1, size);
    }

    CSND_SetPlayState(chn, 1);
    return true;
}

bool ctr::snd::stop(u32 channel) {
    if(!initialized) {
        return false;
    }

    u32 chn = 8 + channel;
    if(!(csndChannels & BIT(chn))) {
        return false;
    }

    CSND_SetPlayState(chn, 0);
    return true;
}

bool ctr::snd::flushCommands() {
    if(!initialized) {
        return false;
    }

    ctr::err::parse((u32) csndExecCmds(0));
    return !ctr::err::has();
}