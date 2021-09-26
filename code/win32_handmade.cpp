#include <windows.h>


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
            OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        
        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
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
            static DWORD paintcolor = WHITENESS;
            PatBlt(deviceContext, X, Y, width, height, paintcolor);
            if (paintcolor == WHITENESS)
                paintcolor = BLACKNESS;
            else 
                paintcolor = WHITENESS;
            EndPaint(windowHandle, &paintStruct);
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
            while (messageReceived = GetMessage(&message, 0, 0, 0))
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

