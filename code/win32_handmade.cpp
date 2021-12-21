#include <windows.h>
#include <cstdint>
#include <xinput.h>
#include <xaudio2.h>
#include <math.h>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint32 bool32;

typedef struct
{
    BITMAPINFO bitmapInfo;
    void *memory;
    int width;
    int height;
    int bitPerPixel;
} bitmap_buffer;

typedef struct 
{
    int width;
    int height;
} dimensions;

struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
    HANDLE hBufferEndEvent;
    StreamingVoiceContext(): hBufferEndEvent(CreateEvent( NULL, FALSE, FALSE, NULL) ){}
    ~StreamingVoiceContext()
    {
        CloseHandle(hBufferEndEvent);
    }
    void OnBufferEnd(void* pBufferContext)
    {
        SetEvent(hBufferEndEvent);
        // free(pBufferContext);
    }
    void OnBufferStart(void *pBufferContext){}
    void OnLoopEnd(void *pBufferContext){}
    void OnStreamEnd(){}
    void OnVoiceError(void *pBufferContext, HRESULT Error){}
    void OnVoiceProcessingPassEnd(){}
    void OnVoiceProcessingPassStart(UINT32 BytesRequired){}
};

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex,XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define XAUDIO2_CREATE(name) HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor)
typedef XAUDIO2_CREATE(xaudio2_create);
XAUDIO2_CREATE(XAudio2CreateStub)
{
    return XAUDIO2_E_DEVICE_INVALIDATED;
}
global_variable xaudio2_create *XAudio2Create_ = XAudio2CreateStub;
#define XAudio2Create XAudio2Create_

#define STREAMING_BUFFER_SIZE 192000
#define MAX_BUFFER_COUNT 2

global_variable bitmap_buffer bitmapBuffer;

global_variable StreamingVoiceContext streamVoiceContext;

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


internal void drawGradientTo(bitmap_buffer *buffer, int xPhase, int yPhase)
{
    int pitch = buffer->bitPerPixel*buffer->width;
    uint8 *row = (uint8 *)buffer->memory;
    for(int y=0; y<buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x=0; x<buffer->width; x++)
        {
            uint8 red = (y + yPhase);
            uint8 green = (x + xPhase);
            *pixel++ = (red << 16) | (green << 8);
        }
        row += pitch;
    }
}


internal void Win32ResizeDIBSection(bitmap_buffer *buffer, int width, int height)
{

    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
    buffer->bitmapInfo.bmiHeader.biWidth = width;
    buffer->bitmapInfo.bmiHeader.biHeight = -height;
    buffer->bitmapInfo.bmiHeader.biPlanes = 1;
    buffer->bitmapInfo.bmiHeader.biBitCount = 32;
    buffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;
    
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
                  &buffer->bitmapInfo,
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


internal void LoadXAudio2Library()
{
    HMODULE XAudio2Lib = LoadLibraryA("XAudio2_9.dll");
    if(XAudio2Lib)
    {
        xaudio2_create *xaudiocreate = (xaudio2_create *) GetProcAddress(XAudio2Lib, "XAudio2Create");
        if(xaudiocreate)
            XAudio2Create = xaudiocreate;
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
                        ySpeed -= 2;
                    else
                        ySpeed += 2;
                }
                else if (VKCode == 'A')
                {
                    if(keyDown)
                        xSpeed -= 2;
                    else
                        xSpeed += 2;
                }
                else if (VKCode == 'S')
                {
                    if(keyDown)
                        ySpeed += 2;
                    else
                        ySpeed -= 2;
                }
                else if (VKCode == 'D')
                {
                    if(keyDown)
                        xSpeed += 2;
                    else
                        xSpeed -= 2;
                }
                else if (VKCode == 'Q')
                {
                    
                }
                else if (VKCode == 'E')
                {
                    
                }
                else if (VKCode == VK_UP)
                {
                    
                }
                else if (VKCode == VK_DOWN)
                {
                    
                }
                else if (VKCode == VK_LEFT)
                {
                    
                }
                else if (VKCode == VK_RIGHT)
                {
                    
                }
                else if (VKCode == VK_ESCAPE)
                {
                    
                }
                else if (VKCode == VK_SPACE)
                {
                    
                }
                else if (VKCode == VK_F4)
                {
                    bool32 altKeyPressed = (VKCode & (1<<29));
                    if (altKeyPressed)
                    {
                        globalRunning = false;
                    }
                }
                else if (VKCode == VK_CONTROL)
                {
                }
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
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to init COM\n");
        return 0;
    }
    
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
            LoadXInputLibrary();
            
            // Audio engine setup
            LoadXAudio2Library(); 
            HRESULT hr; // TODO check every following hr assignment for success
            IXAudio2* pXAudio2 = nullptr;
            
            hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR); // check success
            IXAudio2MasteringVoice* pMasterVoice = nullptr;
            hr = pXAudio2->CreateMasteringVoice( &pMasterVoice ); // check success
        
            WAVEFORMATEX format = {0};
            format.wFormatTag = WAVE_FORMAT_PCM;
            format.nChannels = 2;
            format.nSamplesPerSec = 48000;
            format.wBitsPerSample = 16;
            format.nBlockAlign = (format.wBitsPerSample * format.nChannels) / 8;
            format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
            
            IXAudio2SourceVoice* pSourceVoice;
            hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*) &format, 0, 1.0F, &streamVoiceContext); // check success
            hr = pSourceVoice->Start(0, 0); // check success


            HANDLE heapHandle = GetProcessHeap();
            BYTE *audioBuffers = (BYTE *)HeapAlloc(heapHandle, 
                                              HEAP_ZERO_MEMORY,
                                              MAX_BUFFER_COUNT * STREAMING_BUFFER_SIZE);
            uint32 emptyBufferIndex = 0;
            int waveIndex = 0;
            
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
                        break;
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
                
                // test audio
                if(audioBuffers)
                {
                    uint32 bufferSize = format.nBlockAlign * format.nSamplesPerSec;
                    BYTE * pDataBuffer;
                    uint32 soundHz = 440;
                    uint32 halfWaveletSamples = format.nSamplesPerSec / (2 * soundHz);
                    
                    XAUDIO2_VOICE_STATE state;
                    pSourceVoice->GetState(&state);
                    if(state.BuffersQueued < MAX_BUFFER_COUNT)
                    {
                        // populate pDataBuffer
                        pDataBuffer = &(audioBuffers[STREAMING_BUFFER_SIZE*emptyBufferIndex]);
                        int16 * sampleOut = (int16 *)pDataBuffer;
                        int bytesPerSample = 2;
                        int bufferSampleCount = STREAMING_BUFFER_SIZE / (bytesPerSample * format.nChannels);
                        for(int sampleIndex=0; sampleIndex < bufferSampleCount; ++sampleIndex)
                        {
                            int16 sampleValue = ((waveIndex++ / halfWaveletSamples) % 2) ? 1000 : -1000;
                            *sampleOut++ = sampleValue;
                            *sampleOut++ = sampleValue;
                        }
                        
                        XAUDIO2_BUFFER xaudioBuffer = {0};
                        xaudioBuffer.pContext = &streamVoiceContext;
                        xaudioBuffer.AudioBytes = bufferSize;  //size of the audio buffer in bytes
                        xaudioBuffer.pAudioData = pDataBuffer;  //buffer containing audio data
                        hr = pSourceVoice->SubmitSourceBuffer(&xaudioBuffer); // check success
                        
                        emptyBufferIndex++;
                        emptyBufferIndex %= MAX_BUFFER_COUNT;
                        OutputDebugStringA("buffer submitted");
                    }
                }

                OutputDebugStringA("Out of loop\n");
                drawGradientTo(&bitmapBuffer, xOffset, yOffset);
                
                HDC deviceContext = GetDC(windowHandle);
                
                dimensions rectangleDimensions = getRectangleDimensionsFrom(windowHandle);     
                Win32CopyBufferToWindow(deviceContext, &bitmapBuffer,
                                        rectangleDimensions.width, rectangleDimensions.height);
                
                ReleaseDC(windowHandle, deviceContext);
            }
            
        }
        else return(0);
    }
    else;
    
    return 0;
}

