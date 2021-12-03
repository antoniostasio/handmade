#include <windows.h>
#include <cstdint>
#include <xinput.h>

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


global_variable bitmap_buffer bitmapBuffer;

global_variable bool globalRunning;


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
            uint8 green = (y + yPhase);
            uint8 blue = (x + xPhase);
            *pixel++ = (green << 8) | blue;
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
                    
                }
                else if (VKCode == 'A')
                {
                    
                }
                else if (VKCode == 'S')
                {
                    
                }
                else if (VKCode == 'D')
                {
                    
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
            
            MSG message;
            BOOL messageReceived;
            globalRunning = true;
            int xOffset = 0;
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
                
                // TODO controller input reading
                
                drawGradientTo(&bitmapBuffer, xOffset, xOffset);
                
                HDC deviceContext = GetDC(windowHandle);
                
                dimensions rectangleDimensions = getRectangleDimensionsFrom(windowHandle);     
                Win32CopyBufferToWindow(deviceContext, &bitmapBuffer,
                                        rectangleDimensions.width, rectangleDimensions.height);
                
                ReleaseDC(windowHandle, deviceContext);
                
                xOffset++;
            }
            
        }
        else return(0);
    }
    else;
    
    return 0;
}

