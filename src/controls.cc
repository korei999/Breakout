#include <math.h>

#include "adt/logs.hh"
#include "controls.hh"
#include "frame.hh"

#ifdef __linux__
    #include <linux/input-event-codes.h>
#elif _WIN32
    #undef near
    #undef far
#endif

namespace controls
{

bool g_pressedKeys[300] {};

void
PlayerProcMouse(Player* s)
{
    f64 offsetX = (s->mouse.relX - s->mouse.prevRelX) * s->mouse.sens;
    f64 offsetY = (s->mouse.prevRelY - s->mouse.relY) * s->mouse.sens;

    s->mouse.prevRelX = s->mouse.relX;
    s->mouse.prevRelY = s->mouse.relY;

    s->mouse.yaw += offsetX;
    s->mouse.pitch += offsetY;

    if (s->mouse.pitch > 89.9)
        s->mouse.pitch = 89.9;
    if (s->mouse.pitch < -89.9)
        s->mouse.pitch = -89.9;

    s->front = math::V3Norm({
        f32(cos(math::toRad(s->mouse.yaw)) * cos(math::toRad(s->mouse.pitch))),
        f32(sin(math::toRad(s->mouse.pitch))),
        f32(sin(math::toRad(s->mouse.yaw)) * cos(math::toRad(s->mouse.pitch)))
    });

    s->right = V3Norm(V3Cross(s->front, s->up));
}

void
procKeysOnce(u32 key, u32 pressed)
{
    switch (key)
    {
        case KEY_P:
        case KEY_GRAVE:
            if (pressed)
            {
                frame::g_pApp->bPaused = !frame::g_pApp->bPaused;
                if (frame::g_pApp->bPaused) LOG_WARN("paused: %d\n", frame::g_pApp->bPaused);
            }
            break;

        case KEY_Q:
            if (pressed) AppTogglePointerRelativeMode(frame::g_pApp);
            break;

        case KEY_ESC:
        case KEY_CAPSLOCK:
            if (pressed)
            {
                frame::g_pApp->bRunning = false;
                LOG_OK("quit...\n");
            }
            break;

        case KEY_R:
            /*if (pressed) incCounter = 0;*/
            break;

        case KEY_F:
            if (pressed) AppToggleFullscreen(frame::g_pApp);
            break;

        case KEY_V:
            if (pressed) AppToggleVSync(frame::g_pApp);
            break;

        case KEY_SPACE:
            {
                if (!pressed) break;

                QueuePushBack(&frame::g_projectiles, {
                    .entityIdx = 0,
                    .speed = 1.0f, 
                    .pos = frame::g_player.pos,
                    .dir = math::V2Norm({0.0f, 1.0f}),
                    .bBroken = false
                });
            }
            break;

        default:
            break;
    }
}

void
PlayerProcKeys(Player* s)
{
    PlayerProcMovements(s);

    if (g_pressedKeys[KEY_I])
    {
        frame::g_fov += 100.0f * f32(frame::g_deltaTime);
        LOG_OK("fov: %.3f}\n", frame::g_fov);
    }
    if (g_pressedKeys[KEY_O])
    {
        frame::g_fov -= 100.0f * f32(frame::g_deltaTime);
        LOG_OK("fov: %.3f\n", frame::g_fov);
    }
    if (g_pressedKeys[KEY_Z])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*x += inc * deltaTime;*/
        /*LOG_OK("x: %.3f\n", x);*/
    }
    if (g_pressedKeys[KEY_X])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*y += inc * deltaTime;*/
        /*LOG_OK("y: %.3f\n", y);*/
    }
    if (g_pressedKeys[KEY_C])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*z += inc * deltaTime;*/
        /*LOG_OK("z: %.3f\n", z);*/
    }
}

void
PlayerProcMovements(Player* s)
{
    f64 moveSpeed = s->moveSpeed * frame::g_deltaTime;

    math::V3 combinedMove {};

    if (g_pressedKeys[KEY_A])
    {
        math::V3 left = V3Norm(V3Cross(s->front, s->up));
        combinedMove -= (left);
    }
    if (g_pressedKeys[KEY_D])
    {
        math::V3 left = V3Norm(V3Cross(s->front, s->up));
        combinedMove += (left);
    }
    // if (g_pressedKeys[KEY_W])
    // {
    //     combinedMove += s->up;
    // }
    // if (g_pressedKeys[KEY_S])
    // {
    //     combinedMove -= s->up;
    // }
    f32 len = V3Length(combinedMove);
    if (len > 0) combinedMove = V3Norm(combinedMove, len);

    if (g_pressedKeys[KEY_LEFTSHIFT])
        moveSpeed *= 3;
    if (g_pressedKeys[KEY_LEFTALT])
        moveSpeed /= 3;

    math::V3 newPos = s->pos + (combinedMove * f32(moveSpeed));

    if (newPos.x >= frame::WIDTH - frame::g_unit.x*2) newPos.x = frame::WIDTH - frame::g_unit.x*2;
    if (newPos.x < 0.0f) newPos.x = 0.0f;
    // if (newPos.y >= frame::HEIGHT) newPos.y = frame::HEIGHT;
    // if (newPos.y < 0.0f) newPos.y = 0.0f;

    s->pos = newPos;

    /*COUT("pos: %.2f, %.2f\n", newPos.x, newPos.y);*/
}

void 
PlayerUpdateView(Player* s)
{
    s->view = math::M4LookAt(s->pos, s->pos + s->front, s->up);
}

void 
PlayerUpdateProj(Player* s, f32 fov, f32 aspect, f32 near, f32 far)
{
    s->proj = math::M4Pers(fov, aspect, near, far);
}

} /* namespace controls */
