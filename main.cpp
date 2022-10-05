#include <Windows.h>
#include <iostream>
#include <shellapi.h>
#include <commctrl.h>
#include <tchar.h>
#include <functional>
#include <thread>
#include "FMG.h"
#define WMAPP_NOTIFYCALLBACK (WM_USER +1145)
#define ICON_UID 810
#define HIDE_CONSOLE 1
#define EXIT 2
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void default_func()
{

}
class Application
{
public:
    Application():is_closed(false), is_hide(false)
    {
        
    }
    void SetConsoleInitState(bool is_hide)
    {
        this->is_hide = is_hide;
    }
    bool Init()
    {
        if (!FMGInit()) {
            return false;
        }
        console_hwnd = GetConsoleWindow();
        if (is_hide) {
            ShowWindow(console_hwnd, SW_HIDE);
        }
        
        g_hInst = GetModuleHandle(NULL);
        auto result = CoCreateGuid(&icon_guid);
        bool flag = result == S_OK;
        RegisterWindowClass();
        return flag;
    }
    LRESULT Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_COMMAND:
        {
            int const wmId = HIWORD(wParam);
            switch (wmId) {
            case 0:
            {
                auto readId = LOWORD(wParam);
                switch (readId) {
                case HIDE_CONSOLE:
                    ShowWindow(console_hwnd, SW_HIDE);
                    break;
                case EXIT:
                    Close();
                    break;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
                    break;
                }
            }
                break;
            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        break;
        case WM_CLOSE:
            return DefWindowProc(hwnd, message, wParam, lParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            if (message == icon_message) {
                switch (LOWORD(lParam)) {
                case NIN_SELECT:
                    ShowWindow(console_hwnd, SW_SHOW);
                    break;

                case WM_CONTEXTMENU:
                {
                    POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                    ShowContextMenu(hwnd, pt);
                }
                break;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
                    break;
                }
                break;
            } else {
                return DefWindowProc(hwnd, message, wParam, lParam);
                break;
            }
            
        }
        return 0;
    }
    void Run()
    {
        std::thread t1([this] {
            thread_id = GetCurrentThreadId();
            message_hwnd = CreateWindow(TEXT("MessageWindow"),  
                TEXT("Message"),   
                WS_OVERLAPPEDWINDOW | WS_CAPTION, 
                CW_USEDEFAULT,  
                CW_USEDEFAULT,  
                200,  
                200,  
                NULL, 
                NULL, 
                GetModuleHandle(NULL), 
                NULL  
            );
            ShowWindow(message_hwnd, SW_HIDE);
            auto result = AddNotificationIcon();
            if (!result || !(message_hwnd)) {
                Close();
                return;
            }

            
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        });
        std::thread t2([this] {
            while (!is_closed) {
                if (!CheckContinue()) {
                    Close();
                    break;
                }
                LoopFunc();
            }
        });
        t1.join();
        t2.join();
    }
    ~Application()
    {
        DeleteNotificationIcon();
        FMGEnd();
    }
private:
    bool is_hide;
    DWORD thread_id;
    bool is_closed;
    void Close()
    {
        is_closed = true;
        PostMessage(message_hwnd, WM_CLOSE, 0, 0);
    }
    BOOL AddNotificationIcon()
    {
        icon_message = RegisterWindowMessage(TEXT("114514"));
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = message_hwnd;
        nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
        nid.uCallbackMessage = icon_message;
        nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        nid.guidItem = icon_guid;
        _tcscpy(nid.szTip, TEXT("FanViewer"));
        Shell_NotifyIcon(NIM_ADD, &nid);
        // NOTIFYICON_VERSION_4 is prefered
        nid.uVersion = NOTIFYICON_VERSION_4;
        return Shell_NotifyIcon(NIM_SETVERSION, &nid);
    }
    BOOL DeleteNotificationIcon()
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.uFlags = NIF_GUID;
        nid.guidItem = icon_guid;
        return Shell_NotifyIcon(NIM_DELETE, &nid);
    }
    void RegisterWindowClass()
    {
        WNDCLASS wndclass;
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = WndProc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = GetModuleHandle(NULL);
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName = NULL;
        wndclass.lpszClassName = TEXT("MessageWindow");
        if (!RegisterClass(&wndclass)) {
            //注册窗口类失败时，弹出提示
            MessageBox(NULL, TEXT("This program requires Window NT!"), TEXT("MyClass"), MB_ICONERROR);
            exit(1);
        }
    }
    void ShowContextMenu(HWND hwnd, POINT pt)
    {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu) {
            HMENU hSubMenu = GetSubMenu(hMenu, 0);
            if (hSubMenu) {
                // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON , pt.x, pt.y, 0, hwnd, NULL);
            }
            DestroyMenu(hMenu);
        }
    }
    HMENU CreatePopupMenu()
    {
        HMENU hMenu = CreateMenu();
        HMENU popup = CreateMenu();
        UINT uFlags = MF_POPUP;
        AppendMenu(hMenu, uFlags, (UINT_PTR)popup, NULL);

        uFlags = MF_STRING;
        AppendMenu(popup, uFlags, (UINT_PTR)HIDE_CONSOLE, TEXT("隐藏至托盘"));
        AppendMenu(popup, uFlags, (UINT_PTR)EXIT, TEXT("退出"));
        return hMenu;
    }
    HWND console_hwnd, message_hwnd;
    GUID icon_guid;
    HINSTANCE g_hInst;
    UINT icon_message;
    
};
Application app;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return app.Proc(hwnd, message, wParam, lParam);
}
int main(int argc, char* argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--background") {
        app.SetConsoleInitState(true);
    }
    if (!app.Init()) {
        return 1;
    }
    app.Run();
	return 0;
}
