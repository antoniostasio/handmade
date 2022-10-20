#include "handmade.h"
#include <cstdint>
#include <math.h>

typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;
typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef int64_t     int64;

internal void updateGame(sound_buffer* soundBuffer, int soundSampleCount, bitmap_buffer* mainBuffer, int xOffset, int yOffset)
{
    updateSound(soundBuffer, soundSampleCount);
    
    drawGradientInto(mainBuffer, xOffset, yOffset);
}

internal void drawGradientInto(bitmap_buffer *buffer, int xPhase, int yPhase)
{
    int pitch = buffer->bitPerPixel*buffer->width;
    uint8 *row = (uint8 *)buffer->memory;
    for(int y=0; y<buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x=0; x<buffer->width; x++)
        {
            uint8 green = (y + yPhase);
            uint8 blue = (x + xPhase);
            *pixel++ = (green << 8) | blue;
        }
        row += pitch;
    }
}

internal void updateSound(sound_buffer* soundBuffer, int sampleCount)
{
    int16* bufferCursor = (int16 *)(soundBuffer->buffer);
    
    // Write a full sample
    int maxVolume = 1000;
    int wavePeriodSampleCount = soundBuffer->samplesPerSecond / (soundBuffer->waveHz);
    // Fill region 1
    for(int writeSampleCursor = 0; writeSampleCursor* (soundBuffer->bytesPerSample) < sampleCount; writeSampleCursor++)
    {
        // subdivide a full cicle (2*PI) in n chunks (number of samples in a period)
        // so we get a slice (in time) of the wave
        int16 sampleValue = (int16) (sinf(soundBuffer->waveT) * maxVolume);
        *bufferCursor = sampleValue;
        ++bufferCursor;
        *bufferCursor = sampleValue;
        ++bufferCursor;
        soundBuffer->waveT += (2.f*PI)/(float)wavePeriodSampleCount;
        ++soundBuffer->currentSampleIndex;
    }
}
