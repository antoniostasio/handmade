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



internal void updateGame(sound_buffer* soundBuffer, int soundSampleCount,
                         input_state* gameInput,
                         bitmap_buffer* mainBuffer)
{
    local_persist int yOffset = 0;
    local_persist int xOffset = 0;

    float maxSpeed = 4;
    xOffset -= (gameInput->leftStick.x * maxSpeed);
    yOffset += (gameInput->leftStick.y * maxSpeed);
    
    vec2i speed = {};
    if(gameInput->upButton == button_state::PRESSED)
    {
        speed.y += 2;
    }
    if(gameInput->downButton == button_state::PRESSED)
    {
        speed.y += -2;
    }
    if(gameInput->rightButton == button_state::PRESSED)
    {
        speed.x += -2;
    }
    if(gameInput->leftButton == button_state::PRESSED)
    {
        speed.x += 2;
    }
    
    xOffset += speed.x;
    yOffset += speed.y;
    

    int maxDeltaHZ = 100;
    int waveHzAtRest = 240;
    soundBuffer->waveHz = waveHzAtRest + (float)(maxDeltaHZ)*(gameInput->leftStick.y);
    
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

internal void updateSound(sound_buffer* soundBuffer, int samplesToGenerate)
{
    assert(soundBuffer->bufferSize >= samplesToGenerate * soundBuffer->bytesPerSample);
    int16* bufferCursor = (int16 *)(soundBuffer->buffer);
    
    // Writing n samples for each of th 2 channels
    int maxVolume = 1000;
    int wavePeriodSampleCount = soundBuffer->samplesPerSecond / (soundBuffer->waveHz);
    for(int writeSampleCursor = 0; writeSampleCursor < samplesToGenerate; writeSampleCursor++)
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
