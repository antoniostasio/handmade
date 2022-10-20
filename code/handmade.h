#pragma once

#define global_variable static
#define local_persist   static
#define internal        static


typedef struct
{
    void *memory;
    int width;
    int height;
    int bitPerPixel;
} bitmap_buffer;

typedef struct 
{
    void *buffer;
    int bufferSize;
    int samplesPerSecond;
    int bytesPerSample;
    // int bufferSize;
    // int latency_ms;
    float waveT;
    int currentSampleIndex;
    // int waveHzAtRest;
    int waveHz;
} sound_buffer;

internal void drawGradientInto(bitmap_buffer *buffer, int xPhase, int yPhase);

internal void updateGame(bitmap_buffer* mainBuffer);

internal void updateSound(sound_buffer* soundInfo, int sampleCount);