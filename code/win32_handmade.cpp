#include <windows.h>

#define global_variable static
#define local_persist static
#define internal static

global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;



internal void Win32ResizeDIBSection(int width, int height)
{

    if(bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }
    
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    int bitmapWidth = width;
    int bitmapHeight = height;
    int bitPerPixel = 4;
    int bitmapTotalBytes = bitmapWidth*bitmapHeight*bitPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapTotalBytes, MEM_COMMIT, PAGE_READWRITE);


}


internal void Win32UpdateWindow(HDC deviceContext, int X, int Y, int width, int height)
{
    StretchDIBits(deviceContext,
                  X, Y, width, height,
                  X, Y, width, height,
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
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC deviceContext = BeginPaint(windowHandle, &paintStruct);
            int X = paintStruct.rcPaint.left;
            int Y = paintStruct.rcPaint.top;
            int width = paintStruct.rcPaint.right - X;
            int height = paintStruct.rcPaint.bottom - Y;
            Win32UpdateWindow(deviceContext, X, Y, width, height);
        } break;
        
        default:
        {
            OutputDebugStringA("default\n");
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
            while (running && (messageReceived = GetMessage(&message, 0, 0, 0) ))
            {
                if (messageReceived == -1)
                {
                    // TODO
                    break;
                }
                else
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
            
        }
        else return(0);
    }
    else;
    
    return 0;
}

