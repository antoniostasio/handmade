#pragma once

#define global_variable static
#define local_persist   static
#define internal        static

#define assert(expression) if(!(expression)) {*(int*)0 = 0;}

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

enum button_state
{
    PRESSED,
    RELEASED
};

typedef struct
{
    float x;
    float y;
} vec2f;

typedef struct
{
    double x;
    double y;
} vec2d;


typedef struct
{
    vec2d leftStick;
    vec2d rightStick;
    union
    {
        button_state buttons[8];
        struct
        {
            button_state upButton;
            button_state downButton;
            button_state leftButton;
            button_state rightButton;
            button_state button0;
            button_state button1;
            button_state button2;
            button_state button3;
        };
    };
} input_state;

typedef struct
{
    input_state gamepads[4];
} game_input;

internal void drawGradientInto(bitmap_buffer *buffer, int xPhase, int yPhase);

internal void updateGame(sound_buffer* soundBuffer, int soundSampleCount, 
                         input_state* gameInput,
                         bitmap_buffer* mainBuffer, int xOffset, int yOffset);

internal void updateSound(sound_buffer* soundInfo, int sampleCount);