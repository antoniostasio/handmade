#include <windows.h>
#include <cstdint>
#include <xinput.h>
#include <dsound.h>


#define PI  3.14159265358979323846f   // pi
#define global_variable static
#define local_persist   static
#define internal        static

typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;
typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef int64_t     int64;

typedef uint32 bool32;

typedef struct 
{
    int width;
    int height;
} dimensions;

#include "handmade.cpp"

// Function header macros
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex,XINPUT_STATE *pState)
#define XINPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND8* ppDS, LPUNKNOWN  pUnkOuter)
// Define function type from macro
typedef XINPUT_GET_STATE(xinput_get_state);
typedef XINPUT_SET_STATE(xinput_set_state);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// Stub functions
XINPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

XINPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

// Global function pointers defaulted to stubs
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
// Hide library function names
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// Global variables
global_variable bitmap_buffer bitmapBuffer;
global_variable BITMAPINFO bitmapInfo;
global_variable input_state gameInput{};
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer = 0;
global_variable bool globalRunning;
global_variable int xOffset = 0;
global_variable int yOffset = 0;
global_variable int xSpeed = 0;
global_variable int ySpeed = 0;

internal dimensions getRectangleDimensionsFrom(HWND windowHandle)
{
            dimensions rectDim = {};
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            rectDim.width = clientRect.right - clientRect.left;
            rectDim.height = clientRect.bottom - clientRect.top;
            
            return(rectDim);
}

/**
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
**/

internal void Win32ResizeDIBSection(bitmap_buffer *buffer, int width, int height)
{

    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    buffer->width = width;
    buffer->height = height;
    buffer->bitPerPixel = 4;
    
    int bitmapTotalBytes = buffer->width*buffer->height*buffer->bitPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapTotalBytes, MEM_COMMIT, PAGE_READWRITE);


}


internal void Win32CopyBufferToWindow(HDC deviceContext, bitmap_buffer *buffer,
                                      int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &bitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);

}


internal void LoadXInputLibrary()
{
    HMODULE XInputLib = LoadLibraryA("Xinput1_4.dll");
    if(!XInputLib)
        XInputLib = LoadLibraryA("Xinput1_3.dll");
    if(XInputLib)
    {
        xinput_get_state *xgetstate = (xinput_get_state*) GetProcAddress(XInputLib , "XInputGetState");
        if(xgetstate)
            XInputGetState = xgetstate;
        xinput_set_state *xsetstate = (xinput_set_state*) GetProcAddress(XInputLib , "XInputSetState");
        if(xsetstate)
            XInputSetState = xsetstate;
    }
}


internal void Win32DirectSoundInit(HWND windowHandle, int samplesPerSecond, int bufferSize)
{
    HMODULE DirectSoundLib = LoadLibraryA("Dsound.dll");
    if(DirectSoundLib)
    {
        // First a direct sound object is necessary to get access to the sound device
        LPDIRECTSOUND8 directSound;
        direct_sound_create *DirectSoundCreate = (direct_sound_create*) GetProcAddress(DirectSoundLib, "DirectSoundCreate8");
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, NULL)))
        {
            // After the DSound object has been created we need to set the way the hardware is going to be used
            if(SUCCEEDED(directSound->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY)))
            {
                // A primary buffer is needed to set the format of the audio we're going to write in the actual buffer (secondary)
                LPDIRECTSOUNDBUFFER primaryBuffer;
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    WAVEFORMATEX waveFormat = {};
                    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    waveFormat.nChannels = 2;
                    waveFormat.nSamplesPerSec = samplesPerSecond;
                    waveFormat.wBitsPerSample = 16;
                    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
                    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
                    HRESULT hResult = primaryBuffer->SetFormat(&waveFormat);
                    if(SUCCEEDED(hResult))
                    {
                        bufferDescription = {};
                        bufferDescription.dwSize = sizeof(bufferDescription);
                        bufferDescription.dwBufferBytes = bufferSize;
                        bufferDescription.lpwfxFormat = &waveFormat;
                        directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);
                    }
                }
            }
        }
    }
}

LRESULT CALLBACK MainWindowCallback(HWND   windowHandle,
                                    UINT   messageID,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    LRESULT result = 0;
    
    switch(messageID)
    {
        case WM_SIZE:
        {
            dimensions rectangleDimensions = getRectangleDimensionsFrom(windowHandle);
            HDC deviceContext = GetDC(windowHandle);
            Win32CopyBufferToWindow(deviceContext, &bitmapBuffer, rectangleDimensions.width, rectangleDimensions.height);
            ReleaseDC(windowHandle, deviceContext);  
            // Win32ResizeDIBSection(&bitmapBuffer,
            //                       rectangleDimensions.width, rectangleDimensions.height);
            // OutputDebugStringA("WM_RESIZE\n");
        } break;
        
        case WM_DESTROY:
        {
            globalRunning = false;
        } break;
        
        case WM_CLOSE:
        {
            globalRunning = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            //OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        //case WM_SYSKEYDOWN:
        //case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam;
            bool keyWasDown = (lParam & (1 << 30)) != 0;
            bool keyDown = (lParam & (1 << 31)) == 0;
            if(keyDown != keyWasDown)
            {
                if (VKCode == 'W')
                {
                    if(keyDown)
                    {
                        gameInput.upButton = button_state::PRESSED;
                        ySpeed -= 2;
                    } 
                    else
                    {
                        gameInput.upButton = button_state::RELEASED;
                        ySpeed += 2;
                    }
                }
                else if (VKCode == 'A')
                {
                    if(keyDown)
                    {
                        gameInput.leftButton = button_state::PRESSED;
                        xSpeed -= 2;
                    }
                    else
                    {
                        gameInput.leftButton = button_state::RELEASED;
                        xSpeed += 2;
                    }
                }
                else if (VKCode == 'S')
                {
                    if(keyDown)
                    {
                        gameInput.downButton = button_state::PRESSED;
                        ySpeed += 2;
                    }
                    else
                    {
                        gameInput.downButton = button_state::RELEASED;
                        ySpeed -= 2;
                    }
                }
                else if (VKCode == 'D')
                {
                    if(keyDown)
                    {
                        gameInput.rightButton = button_state::PRESSED;
                        xSpeed += 2;
                    }
                    else
                    {
                        gameInput.rightButton = button_state::RELEASED;
                        xSpeed -= 2;
                    }
                }
                else if (VKCode == 'Q')
                {}
                else if (VKCode == 'E')
                {}
                else if (VKCode == VK_UP)
                {}
                else if (VKCode == VK_DOWN)
                {}
                else if (VKCode == VK_LEFT)
                {}
                else if (VKCode == VK_RIGHT)
                {}
                else if (VKCode == VK_ESCAPE)
                {}
                else if (VKCode == VK_SPACE)
                {}
                else if (VKCode == VK_F4)
                {
                    bool32 altKeyPressed = (VKCode & (1<<29));
                    if (altKeyPressed)
                    {
                        globalRunning = false;
                    }
                }
                else if (VKCode == VK_CONTROL)
                {}
            }
            
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC deviceContext = BeginPaint(windowHandle, &paintStruct);

            dimensions rectangleDimensions = getRectangleDimensionsFrom(windowHandle);
            Win32CopyBufferToWindow(deviceContext, &bitmapBuffer,
                                    rectangleDimensions.width, rectangleDimensions.height);
            EndPaint(windowHandle, &paintStruct);
            // OutputDebugStringA("WM_PAINT\n");
        } break;
        
        default:
        {
            //OutputDebugStringA("default\n");
            result = DefWindowProcA(windowHandle, messageID, wParam, lParam);
        } break;
    }
    
    return result;
}


int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PWSTR pCmdLine,
                    int nCmdShow)
{
    Win32ResizeDIBSection(&bitmapBuffer, 1280, 720);
    
    // Window properties definition
    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC |CS_HREDRAW |CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = hInstance;
    // windowClass.hIcon;
    // windowClass.hCursor;
    windowClass.lpszClassName = "MainWindowClass";
    
    ATOM registered = 0;
    if(registered = RegisterClassExA(&windowClass) )
    {
        HWND windowHandle = CreateWindowExA(0,
                                            windowClass.lpszClassName,
                                            "Handmade",
                                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                            0,
                                            0,
                                            hInstance,
                                            0);
        if (windowHandle)
        {
            input_state gamepadInput{};
            LoadXInputLibrary();
            
            sound_buffer soundBuffer;
            soundBuffer.samplesPerSecond = 48000;
            soundBuffer.bytesPerSample = sizeof(int16) * 2;
            soundBuffer.waveT = 0;
            soundBuffer.currentSampleIndex = 0;
            soundBuffer.waveHz = 240;
            
            int latency_sampleCount =(soundBuffer.samplesPerSecond/1000)*60;
            soundBuffer.bufferSize = latency_sampleCount*soundBuffer.bytesPerSample;
            soundBuffer.buffer = VirtualAlloc(0, soundBuffer.bufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            int DSbufferSize = 2 * soundBuffer.bytesPerSample * soundBuffer.samplesPerSecond;
            
            Win32DirectSoundInit(windowHandle, soundBuffer.samplesPerSecond, DSbufferSize);
            
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            DWORD previousPlayCursor = 0;
            
            MSG message;
            BOOL messageReceived;
            globalRunning = true;

            while (globalRunning)
            {
                while(messageReceived = PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                    {
                    if (messageReceived == -1)
                    {
                        // TODO
                    }
                    else
                    {
                        if(message.message == WM_QUIT)
                        {
                            globalRunning = false;
                        }
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }
                }
                
                // Controller input reading
                DWORD dwResult;    
                for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
                {
                    XINPUT_STATE state = {};
                    
                    // Simply get the state of the controller from XInput.
                    dwResult = XInputGetState( i, &state );
                    
                    if( dwResult == ERROR_SUCCESS )
                    {
                        XINPUT_GAMEPAD *gamepad = &(state.Gamepad);
                        WORD buttonPressed = gamepad->wButtons;
                        if(buttonPressed & XINPUT_GAMEPAD_DPAD_UP)
                        {
                            --yOffset;
                        }
                        if(buttonPressed & XINPUT_GAMEPAD_DPAD_DOWN)
                        {
                            ++yOffset;
                        }
                        if(buttonPressed & XINPUT_GAMEPAD_DPAD_LEFT)
                        {
                            --xOffset;
                        }
                        if(buttonPressed & XINPUT_GAMEPAD_DPAD_RIGHT)
                        {
                            ++xOffset;
                        }
                        
                        
                        double lx = gamepad->sThumbLX;
                        double ly = gamepad->sThumbLY;
                        double magnitude = sqrt(lx*lx + ly*ly);
                        
                        if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                        {
                            gameInput.leftStick.x = lx;
                            gameInput.leftStick.y = ly;
                            xOffset += (lx * 0.0001);
                            yOffset += (ly * 0.0001);
                        }
                    }
                    else
                    {
                        // Controller is not connected
                    }
                }
                xOffset += xSpeed;
                yOffset += ySpeed;
                
                
                // NOTE: testing audio
                // Before writing in the audio buffer it's necessary to find 
                // the locations where it can and should be written and lock those
                DWORD currentPlayCursor;
                DWORD safeToWriteCursor;
                globalSecondaryBuffer->GetCurrentPosition(&currentPlayCursor, &safeToWriteCursor);
                // Offset at which to start lock,  Size custom defined by latency value
                // TODO: change lock start to playcursor + latency (also need to find waveT at lockStart)

                // when currentPlayCursor is close to the end of the buffer or has just wrapped, the desired write position
                // can be smaller than the last written position if this didn't wrap in the previous cycle
                DWORD samplesPlayed = 0;
                if (currentPlayCursor >= previousPlayCursor)
                    samplesPlayed = (currentPlayCursor-previousPlayCursor)/soundBuffer.bytesPerSample;
                else
                    samplesPlayed = ((DSbufferSize - previousPlayCursor) + currentPlayCursor)/soundBuffer.bytesPerSample;
                    
                previousPlayCursor = currentPlayCursor;
                
                
                DWORD lockSampleCount = 0;
                if(samplesPlayed < latency_sampleCount)
                    lockSampleCount = samplesPlayed;
                else
                    lockSampleCount = latency_sampleCount;

                updateGame(&soundBuffer, lockSampleCount, &gameInput, &bitmapBuffer, xOffset, yOffset);
                
                // Copying game generated sound inside platform buffer.
                // Note: circular buffer can have locked portion split in two parts because of wrapping.
                DWORD  desiredWritePosition = (currentPlayCursor + latency_sampleCount*soundBuffer.bytesPerSample) % DSbufferSize;
                DWORD  lockSize = lockSampleCount * soundBuffer.bytesPerSample;
                LPVOID region1Location = 0;
                DWORD  region1ByteSize = 0;
                LPVOID region2Location = 0;
                DWORD  region2ByteSize = 0;
                HRESULT lockResult = globalSecondaryBuffer->Lock(desiredWritePosition, lockSize,     
                                                         &region1Location, &region1ByteSize,  
                                                         &region2Location, &region2ByteSize,  
                                                         0 /** :Flag **/);
                if (DS_OK == lockResult)
                {   
                    int16* sourceBufferCursor      = (int16*)soundBuffer.buffer;
                    int16* destinationBufferCursor = (int16*)region1Location;
                    for(int sampleNum = 0; sampleNum < region1ByteSize/soundBuffer.bytesPerSample; ++sampleNum)
                    {
                        *destinationBufferCursor = *sourceBufferCursor;
                        ++destinationBufferCursor;
                        ++sourceBufferCursor;
                        *destinationBufferCursor = *sourceBufferCursor;
                        ++destinationBufferCursor;
                        ++sourceBufferCursor;
                    }
                    destinationBufferCursor = (int16*)region2Location;
                    for(int sampleNum = 0; sampleNum < region2ByteSize/soundBuffer.bytesPerSample; ++sampleNum)
                    {
                        *destinationBufferCursor = *sourceBufferCursor;
                        ++destinationBufferCursor;
                        ++sourceBufferCursor;
                        *destinationBufferCursor = *sourceBufferCursor;
                        ++destinationBufferCursor;
                        ++sourceBufferCursor;
                    }
                    globalSecondaryBuffer->Unlock(region1Location, region1ByteSize, region2Location, region2ByteSize);
                }
                
                HDC deviceContext = GetDC(windowHandle);
                
                dimensions rectangleDimensions = getRectangleDimensionsFrom(windowHandle);     
                Win32CopyBufferToWindow(deviceContext, &bitmapBuffer,
                                        rectangleDimensions.width, rectangleDimensions.height);
                                        
                ReleaseDC(windowHandle, deviceContext);
            }
            
        }
        // no window handle
        else return(0);
    }
    else;
    
    return 0;
}

