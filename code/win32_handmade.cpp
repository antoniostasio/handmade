#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PWSTR pCmdLine,
                    int nCmdShow)
{
    MessageBox(0,"Useless message","Handmade", MB_OK|MB_ICONINFORMATION);
    return(0);
}