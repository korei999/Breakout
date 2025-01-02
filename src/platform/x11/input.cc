#include "input.hh"

#include "adt/defer.hh"
#include "adt/logs.hh"
#include "app.hh"
#include "controls.hh"
#include "adt/Pair.hh"

#include <X11/keysym.h>

#include <fcntl.h>
#include <linux/input.h>

namespace platform
{
namespace x11
{
namespace input
{

static int s_aKeyCodeToLinux[300] {};

static Atom s_atomWM_PROTOCOLS;
static Atom s_atomWM_DELETE_WINDOW;

void
startMouseReadingThread(Win* s)
{
    thrd_t mouseThrd {};

    thrd_create(&mouseThrd,
        +[](void* pArg) -> int
        {
            int fdMouse = open("/dev/input/mice", O_RDONLY);
            if (fdMouse < 0)
                LOG_FATAL("open(\"/dev/input/mice\")\n");
            defer( close(fdMouse) );

            s8 aBuff[3] {};
            Pair<s8, s8> xy {};
            auto& win = *(Win*)pArg;

            while (win.m_bRunning)
            {
                int nRead = read(fdMouse, &aBuff, sizeof(aBuff));
                if (nRead == 3)
                {
                    auto& [x, y] = xy;
                    x = aBuff[1];
                    y = aBuff[2];

                    if (win.m_bPointerRelativeMode)
                    {
                        controls::g_mouse.relX += (f64)x;
                        controls::g_mouse.relY += (f64)y;
                    }
                }
            }

            return thrd_success;
        },
        s
    );

    thrd_detach(mouseThrd);
}

void
mapX11KeycodesToLinuxKeycodes(Win* s)
{
    auto& aMap = s_aKeyCodeToLinux;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Escape)] = KEY_ESC;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Caps_Lock)] = KEY_CAPSLOCK;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Shift_L)] = KEY_LEFTSHIFT;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Shift_R)] = KEY_RIGHTSHIFT;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Alt_L)] = KEY_LEFTALT;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Alt_R)] = KEY_RIGHTALT;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Control_L)] = KEY_LEFTCTRL;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Control_R)] = KEY_RIGHTCTRL;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Meta_L)] = KEY_LEFTMETA;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Meta_R)] = KEY_RIGHTMETA;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_space)] = KEY_SPACE;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_a)] = KEY_A;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_b)] = KEY_B;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_c)] = KEY_C;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_d)] = KEY_D;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_e)] = KEY_E;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_f)] = KEY_F;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_g)] = KEY_G;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_h)] = KEY_H;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_i)] = KEY_I;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_j)] = KEY_I;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_k)] = KEY_K;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_l)] = KEY_L;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_m)] = KEY_M;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_n)] = KEY_N;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_o)] = KEY_O;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_p)] = KEY_P;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_q)] = KEY_Q;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_r)] = KEY_R;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_s)] = KEY_S;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_t)] = KEY_T;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_u)] = KEY_U;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_v)] = KEY_V;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_w)] = KEY_W;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_x)] = KEY_X;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_y)] = KEY_Y;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_z)] = KEY_Z;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_A)] = KEY_A;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_B)] = KEY_B;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_C)] = KEY_C;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_D)] = KEY_D;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_E)] = KEY_E;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F)] = KEY_F;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_G)] = KEY_G;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_H)] = KEY_H;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_I)] = KEY_I;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_J)] = KEY_I;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_K)] = KEY_K;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_L)] = KEY_L;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_M)] = KEY_M;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_N)] = KEY_N;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_O)] = KEY_O;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_P)] = KEY_P;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Q)] = KEY_Q;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_R)] = KEY_R;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_S)] = KEY_S;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_T)] = KEY_T;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_U)] = KEY_U;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_V)] = KEY_V;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_W)] = KEY_W;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_X)] = KEY_X;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Y)] = KEY_Y;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_Z)] = KEY_Z;

    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F1)] = KEY_F1;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F2)] = KEY_F2;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F3)] = KEY_F3;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F4)] = KEY_F4;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F5)] = KEY_F5;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F6)] = KEY_F6;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F7)] = KEY_F7;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F8)] = KEY_F8;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F9)] = KEY_F9;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F10)] = KEY_F10;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F11)] = KEY_F11;
    aMap[XKeysymToKeycode(s->m_pDisplay, XK_F12)] = KEY_F12;
}

void
storeAtoms(Win* s)
{
    s_atomWM_PROTOCOLS = XInternAtom(s->m_pDisplay, "WM_PROTOCOLS", false);
    s_atomWM_DELETE_WINDOW = XInternAtom(s->m_pDisplay, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(s->m_pDisplay, s->m_window, &s_atomWM_DELETE_WINDOW, 1);
}

static void
setMods()
{
}

void
procEvents(Win* s)
{
    while (XPending(s->m_pDisplay))
    {
        XEvent event;
        XNextEvent(s->m_pDisplay, &event);

        auto& l = event.xclient.data.l[0];

        switch (event.type)
        {
            default: break;

            case Expose:
            {
                LOG("Expose\n");
            }
            break;

            case MotionNotify:
            {
                const auto& m = event.xmotion;

                controls::g_mouse.absX = (f64)m.x;
                controls::g_mouse.absY = (f64)m.y;
            }
            break;

            case KeyPress:
            {
                auto linuxCode = s_aKeyCodeToLinux[event.xkey.keycode];

                controls::g_aPressedKeys[linuxCode] = true;
                controls::g_eKeyMods = MOD_STATE(event.xkey.state);
            }
            break;

            case KeyRelease:
            {
                auto linuxCode = s_aKeyCodeToLinux[event.xkey.keycode];

                controls::g_aPressedKeys[linuxCode] = false;
                controls::g_eKeyMods = ~MOD_STATE(event.xkey.state);
            }
            break;

            case ButtonPress:
            {
                LOG("ButtonPress: {}\n", event.xbutton.button);
            }
            break;

            case ButtonRelease:
            {
            }
            break;

            case ClientMessage:
            {
                char* pAtomName = XGetAtomName(s->m_pDisplay, event.xclient.data.l[0]);
                LOG("atom({}) : '{}'\n", l, pAtomName);
                defer( XFree(pAtomName) );
                
                if (l == (long)s_atomWM_DELETE_WINDOW)
                {
                    s->m_bRunning = false;
                    app::g_pMixer->m_bRunning = false;
                }
            }
            break;

            case ConfigureNotify:
            {
                XWindowAttributes attr;
                Status status = XGetWindowAttributes(s->m_pDisplay, s->m_window, &attr);

                s->m_wWidth = attr.width;
                s->m_wHeight = attr.height;
            }
            break;
        }
    }
}

} /* namespace input */
} /* namespace x11 */
} /* namespace platform */
