#include "input.hh"

#include "adt/defer.hh"
#include "adt/logs.hh"
#include "app.hh"
#include "controls.hh"

#include <X11/keysym.h>

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
mapX11KeycodesToLinuxKeycodes(Window* s)
{
    auto& aMap = s_aKeyCodeToLinux;

    aMap[XKeysymToKeycode(s->pDisplay, XK_Escape)] = KEY_ESC;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Caps_Lock)] = KEY_CAPSLOCK;

    aMap[XKeysymToKeycode(s->pDisplay, XK_Shift_L)] = KEY_LEFTSHIFT;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Shift_R)] = KEY_RIGHTSHIFT;

    aMap[XKeysymToKeycode(s->pDisplay, XK_Alt_L)] = KEY_LEFTALT;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Alt_R)] = KEY_RIGHTALT;

    aMap[XKeysymToKeycode(s->pDisplay, XK_Control_L)] = KEY_LEFTCTRL;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Control_R)] = KEY_RIGHTCTRL;

    aMap[XKeysymToKeycode(s->pDisplay, XK_Meta_L)] = KEY_LEFTMETA;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Meta_R)] = KEY_RIGHTMETA;

    aMap[XKeysymToKeycode(s->pDisplay, XK_space)] = KEY_SPACE;

    aMap[XKeysymToKeycode(s->pDisplay, XK_a)] = KEY_A;
    aMap[XKeysymToKeycode(s->pDisplay, XK_b)] = KEY_B;
    aMap[XKeysymToKeycode(s->pDisplay, XK_c)] = KEY_C;
    aMap[XKeysymToKeycode(s->pDisplay, XK_d)] = KEY_D;
    aMap[XKeysymToKeycode(s->pDisplay, XK_e)] = KEY_E;
    aMap[XKeysymToKeycode(s->pDisplay, XK_f)] = KEY_F;
    aMap[XKeysymToKeycode(s->pDisplay, XK_g)] = KEY_G;
    aMap[XKeysymToKeycode(s->pDisplay, XK_h)] = KEY_H;
    aMap[XKeysymToKeycode(s->pDisplay, XK_i)] = KEY_I;
    aMap[XKeysymToKeycode(s->pDisplay, XK_j)] = KEY_I;
    aMap[XKeysymToKeycode(s->pDisplay, XK_k)] = KEY_K;
    aMap[XKeysymToKeycode(s->pDisplay, XK_l)] = KEY_L;
    aMap[XKeysymToKeycode(s->pDisplay, XK_m)] = KEY_M;
    aMap[XKeysymToKeycode(s->pDisplay, XK_n)] = KEY_N;
    aMap[XKeysymToKeycode(s->pDisplay, XK_o)] = KEY_O;
    aMap[XKeysymToKeycode(s->pDisplay, XK_p)] = KEY_P;
    aMap[XKeysymToKeycode(s->pDisplay, XK_q)] = KEY_Q;
    aMap[XKeysymToKeycode(s->pDisplay, XK_r)] = KEY_R;
    aMap[XKeysymToKeycode(s->pDisplay, XK_s)] = KEY_S;
    aMap[XKeysymToKeycode(s->pDisplay, XK_t)] = KEY_T;
    aMap[XKeysymToKeycode(s->pDisplay, XK_u)] = KEY_U;
    aMap[XKeysymToKeycode(s->pDisplay, XK_v)] = KEY_V;
    aMap[XKeysymToKeycode(s->pDisplay, XK_w)] = KEY_W;
    aMap[XKeysymToKeycode(s->pDisplay, XK_x)] = KEY_X;
    aMap[XKeysymToKeycode(s->pDisplay, XK_y)] = KEY_Y;
    aMap[XKeysymToKeycode(s->pDisplay, XK_z)] = KEY_Z;

    aMap[XKeysymToKeycode(s->pDisplay, XK_A)] = KEY_A;
    aMap[XKeysymToKeycode(s->pDisplay, XK_B)] = KEY_B;
    aMap[XKeysymToKeycode(s->pDisplay, XK_C)] = KEY_C;
    aMap[XKeysymToKeycode(s->pDisplay, XK_D)] = KEY_D;
    aMap[XKeysymToKeycode(s->pDisplay, XK_E)] = KEY_E;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F)] = KEY_F;
    aMap[XKeysymToKeycode(s->pDisplay, XK_G)] = KEY_G;
    aMap[XKeysymToKeycode(s->pDisplay, XK_H)] = KEY_H;
    aMap[XKeysymToKeycode(s->pDisplay, XK_I)] = KEY_I;
    aMap[XKeysymToKeycode(s->pDisplay, XK_J)] = KEY_I;
    aMap[XKeysymToKeycode(s->pDisplay, XK_K)] = KEY_K;
    aMap[XKeysymToKeycode(s->pDisplay, XK_L)] = KEY_L;
    aMap[XKeysymToKeycode(s->pDisplay, XK_M)] = KEY_M;
    aMap[XKeysymToKeycode(s->pDisplay, XK_N)] = KEY_N;
    aMap[XKeysymToKeycode(s->pDisplay, XK_O)] = KEY_O;
    aMap[XKeysymToKeycode(s->pDisplay, XK_P)] = KEY_P;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Q)] = KEY_Q;
    aMap[XKeysymToKeycode(s->pDisplay, XK_R)] = KEY_R;
    aMap[XKeysymToKeycode(s->pDisplay, XK_S)] = KEY_S;
    aMap[XKeysymToKeycode(s->pDisplay, XK_T)] = KEY_T;
    aMap[XKeysymToKeycode(s->pDisplay, XK_U)] = KEY_U;
    aMap[XKeysymToKeycode(s->pDisplay, XK_V)] = KEY_V;
    aMap[XKeysymToKeycode(s->pDisplay, XK_W)] = KEY_W;
    aMap[XKeysymToKeycode(s->pDisplay, XK_X)] = KEY_X;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Y)] = KEY_Y;
    aMap[XKeysymToKeycode(s->pDisplay, XK_Z)] = KEY_Z;

    aMap[XKeysymToKeycode(s->pDisplay, XK_F1)] = KEY_F1;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F2)] = KEY_F2;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F3)] = KEY_F3;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F4)] = KEY_F4;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F5)] = KEY_F5;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F6)] = KEY_F6;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F7)] = KEY_F7;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F8)] = KEY_F8;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F9)] = KEY_F9;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F10)] = KEY_F10;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F11)] = KEY_F11;
    aMap[XKeysymToKeycode(s->pDisplay, XK_F12)] = KEY_F12;
}

void
storeAtoms(Window* s)
{
    s_atomWM_PROTOCOLS = XInternAtom(s->pDisplay, "WM_PROTOCOLS", false);
    s_atomWM_DELETE_WINDOW = XInternAtom(s->pDisplay, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(s->pDisplay, s->window, &s_atomWM_DELETE_WINDOW, 1);
}

void
procEvents(Window* s)
{
    while (XPending(s->pDisplay))
    {
        XEvent event;
        XNextEvent(s->pDisplay, &event);

        auto& l = event.xclient.data.l[0];

        switch (event.type)
        {
            default: break;

            case Expose: {
                LOG("Expose\n");
            } break;

            case MotionNotify: {
                auto& m = event.xmotion;

                if (s->m_bPointerRelativeMode)
                {
                    controls::g_mouse.relX += (f64)m.x;
                    controls::g_mouse.relY += (f64)m.y;
                }
                else
                {
                    controls::g_mouse.absX = (f64)m.x;
                    controls::g_mouse.absY = (f64)m.y;
                }

            } break;

            case KeyPress: {
                auto linuxCode = s_aKeyCodeToLinux[event.xkey.keycode];

                controls::g_aPressedKeys[linuxCode] = true;
                controls::procKeysOnce(linuxCode, true);
            } break;

            case KeyRelease: {
                auto linuxCode = s_aKeyCodeToLinux[event.xkey.keycode];

                controls::g_aPressedKeys[linuxCode] = false;
                controls::procKeysOnce(linuxCode, false);
            } break;

            case ButtonPress: {
                LOG("ButtonPress: {}\n", event.xbutton.button);
            } break;

            case ButtonRelease: {
            } break;

            case ClientMessage: {

                char* pAtomName = XGetAtomName(s->pDisplay, event.xclient.data.l[0]);
                LOG("atom({}) : '{}'\n", l, pAtomName);
                defer( XFree(pAtomName) );
                
                if (l == (long)s_atomWM_DELETE_WINDOW)
                {
                    s->m_bRunning = false;
                    app::g_pMixer->m_bRunning = false;
                }
            } break;

            case ConfigureNotify: {
                XWindowAttributes attr;
                Status status = XGetWindowAttributes(s->pDisplay, s->window, &attr);

                s->m_wWidth = attr.width;
                s->m_wHeight = attr.height;
            } break;
        }
    }
}

} /* namespace input */
} /* namespace x11 */
} /* namespace platform */
