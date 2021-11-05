#include <windows.h>
#include <cstdint>

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

global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bitPerPixel;


internal void drawGradient(int xOffset, int yOffset)
{
    int pitch = bitPerPixel*bitmapWidth;
    uint8 *row = (uint8 *)bitmapMemory;
    for(int y=0; y<bitmapHeight; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x=0; x<bitmapWidth; x++)
        {
            uint8 green = (y + yOffset);
            uint8 blue = (x + xOffset);
            *pixel++ = (green << 8) | blue;
        }
        row += pitch;
    }
}


internal void Win32ResizeDIBSection(int width, int height)
{

    if(bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }
    
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    bitmapWidth = width;
    bitmapHeight = height;
    bitPerPixel = 4;
    int bitmapTotalBytes = bitmapWidth*bitmapHeight*bitPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapTotalBytes, MEM_COMMIT, PAGE_READWRITE);


}


internal void Win32UpdateWindow(HDC deviceContext, RECT windowRect, int X, int Y, int width, int height)
{
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeith = windowRect.bottom - windowRect.top;
    StretchDIBits(deviceContext,
                  0, 0, bitmapWidth, bitmapHeight,
                  0, 0, windowWidth, windowHeith,
                  bitmapMemory,
                  &bitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);

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
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            
            Win32ResizeDIBSection(width, height);
        } break;
        
        case WM_DESTROY:
        {
            running = false;
        } break;
        
        case WM_CLOSE:
        {
            running = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            //OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            PAINTSTRUCT paintStruct;
            HDC deviceContext = BeginPaint(windowHandle, &paintStruct);
            int X = paintStruct.rcPaint.left;
            int Y = paintStruct.rcPaint.top;
            int width = paintStruct.rcPaint.right - X;
            int height = paintStruct.rcPaint.bottom - Y;
            // drawGradient(0,0);
            Win32UpdateWindow(deviceContext, clientRect, 0, 0, width, height);
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
            MSG message;
            BOOL messageReceived;
            running = true;
            int xOffset = 0;
            while (running)
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
                            running = false;
                        }
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }
                }
                drawGradient(xOffset, xOffset);
                HDC deviceContext = GetDC(windowHandle);
                RECT clientRect;
                GetClientRect(windowHandle, &clientRect);
                int X = clientRect.left;
                int Y = clientRect.top;
                int width = clientRect.right - X;
                int height = clientRect.bottom - Y;
                Win32UpdateWindow(deviceContext, clientRect, X, Y, width, height);
                xOffset++;
            }
            
        }
        else return(0);
    }
    else;
    
    return 0;
}

