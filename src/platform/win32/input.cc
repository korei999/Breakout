#include "input.hh"

#include "adt/logs.hh"
#include "controls.hh" 
#include "app.hh"

#define WIN32_LEAN_AND_MEAN 1
#include <windowsx.h>

#ifdef near
    #undef near
#endif
#ifdef far
    #undef far
#endif

namespace platform
{
namespace win32
{
namespace input
{

int aAsciiToLinuxKeyCodes[300] {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    KEY_TAB,
    0,
    0,
    0,
    0,
    KEY_LEFTSHIFT,
    KEY_LEFTCTRL,
    KEY_LEFTALT,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    KEY_ESC,
    0,
    0,
    0,
    0,
    KEY_SPACE,
    KEY_1,
    KEY_APOSTROPHE,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_7,
    KEY_APOSTROPHE,
    KEY_9,
    KEY_0,
    KEY_8,
    KEY_EQUAL,
    KEY_COMMA,
    KEY_MINUS,
    KEY_DOT,
    KEY_SLASH,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_SEMICOLON,
    KEY_SEMICOLON,
    KEY_COMMA,
    KEY_EQUAL,
    KEY_DOT,
    KEY_SLASH,
    KEY_2,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_O,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_LEFTBRACE,
    KEY_BACKSLASH,
    KEY_RIGHTBRACE,
    KEY_6,
    KEY_MINUS,
    KEY_GRAVE,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_O,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_LEFTBRACE,
    KEY_BACKSLASH,
    KEY_RIGHTBRACE,
    KEY_GRAVE,
    KEY_DELETE,
};

/* https://gist.github.com/luluco250/ac79d72a734295f167851ffdb36d77ee */

void
registerRawMouseDevice(Win32Window* pApp, bool on)
{
    DWORD flag = on ? RIDEV_NOLEGACY : RIDEV_REMOVE;

    pApp->rawInputDevices[0].usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
    pApp->rawInputDevices[0].usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
    pApp->rawInputDevices[0].dwFlags = flag;     /* adds mouse and also ignores legacy mouse messages */
    pApp->rawInputDevices[0].hwndTarget = 0;

    // pApp->rawInputDevices[1].usUsagePage = 0x01;       /* HID_USAGE_PAGE_GENERIC */
    // pApp->rawInputDevices[1].usUsage = 0x06;           /* HID_USAGE_GENERIC_KEYBOARD */
    // pApp->rawInputDevices[1].dwFlags = RIDEV_NOLEGACY; /* adds keyboard and also ignores legacy keyboard messages */
    // pApp->rawInputDevices[1].hwndTarget = 0;

    if (RegisterRawInputDevices(pApp->rawInputDevices, 1, sizeof(pApp->rawInputDevices[0])) == FALSE)
        LOG_FATAL("RegisterRawInputDevices failed: {}\n", GetLastError());
}

void
registerRawKBDevice(Win32Window* pApp, bool on)
{
    DWORD flag = on ? RIDEV_NOLEGACY : RIDEV_REMOVE;

    pApp->rawInputDevices[1].usUsagePage = 0x01;       /* HID_USAGE_PAGE_GENERIC */
    pApp->rawInputDevices[1].usUsage = 0x06;           /* HID_USAGE_GENERIC_KEYBOARD */
    pApp->rawInputDevices[1].dwFlags = flag; /* adds keyboard and also ignores legacy keyboard messages */
    pApp->rawInputDevices[1].hwndTarget = 0;

    if (RegisterRawInputDevices(pApp->rawInputDevices, 1, sizeof(pApp->rawInputDevices[1])) == FALSE)
        LOG_FATAL("RegisterRawInputDevices failed: {}\n", GetLastError());
}

bool
enterFullscreen(HWND hwnd, int fullscreenWidth, int fullscreenHeight, int colorBits, int refreshRate)
{
    DEVMODE fullscreenSettings;
    bool bSucces;

    EnumDisplaySettings(NULL, 0, &fullscreenSettings);
    fullscreenSettings.dmPelsWidth = fullscreenWidth;
    fullscreenSettings.dmPelsHeight = fullscreenHeight;
    fullscreenSettings.dmBitsPerPel = colorBits;
    fullscreenSettings.dmDisplayFrequency = refreshRate;
    fullscreenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, fullscreenWidth, fullscreenHeight, SWP_SHOWWINDOW);
    bSucces = ChangeDisplaySettings(&fullscreenSettings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
    ShowWindow(hwnd, SW_MAXIMIZE);

    return bSucces;
}

bool
exitFullscreen(HWND hwnd, int windowX, int windowY, int windowedWidth, int windowedHeight, int windowedPaddingX, int windowedPaddingY)
{
    bool bSucces;

    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LEFT);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    bSucces = ChangeDisplaySettings(NULL, CDS_RESET) == DISP_CHANGE_SUCCESSFUL;
    SetWindowPos(hwnd, HWND_NOTOPMOST, windowX, windowY, windowedWidth + windowedPaddingX, windowedHeight + windowedPaddingY, SWP_SHOWWINDOW);
    ShowWindow(hwnd, SW_RESTORE);

    return bSucces;
}

LRESULT CALLBACK
windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win32Window* s = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_DESTROY:
            s->m_bRunning = false;
            app::g_pMixer->m_bRunning = false;
            return 0;

        case WM_SIZE:
            s->m_wWidth = LOWORD(lParam);
            s->m_wHeight = HIWORD(lParam);
            break;

        case WM_KILLFOCUS:
            memset(controls::g_aPressedKeys, 0, sizeof(controls::g_aPressedKeys));
            break;

        case WM_NCCREATE:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
            break;

        case WM_LBUTTONDOWN:
            break;

        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_KEYDOWN:
            {
                WPARAM keyCode = wParam;
                bool bWasDown = ((lParam & (1 << 30)) != 0);
                bool bDown = ((lParam >> 31) & 1) == 0;

                if (bWasDown == bDown)
                    break;

                controls::g_aPressedKeys[ aAsciiToLinuxKeyCodes[keyCode] ] = bDown;
                controls::procKeysOnce(aAsciiToLinuxKeyCodes[keyCode], bDown);
            }
            break;

        case WM_MOUSEMOVE:
            {
                controls::g_mouse.absX = GET_X_LPARAM(lParam);
                controls::g_mouse.absY = GET_Y_LPARAM(lParam);
            }
            break;

        case WM_INPUT:
            {
                u32 size = sizeof(RAWINPUT);
                static RAWINPUT raw[sizeof(RAWINPUT)] {};
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    controls::g_mouse.relX += raw->data.mouse.lLastX;
                    controls::g_mouse.relY += raw->data.mouse.lLastY;
                }
            }
            break;

        default:
            break;
    }

    if (s && s->m_bPointerRelativeMode)
    {
        RECT r;
        GetWindowRect(s->hWindow, &r);

        SetCursorPos(
            s->m_wWidth / 2 + r.left,
            s->m_wHeight / 2 + r.top
        );
        SetCursor(nullptr);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} /* namespace input */
} /* namespace win32 */
} /* namespace platform */
