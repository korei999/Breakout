#include "frame.hh"

#include "adt/Arena.hh"
#include "adt/defer.hh"
#include "adt/logs.hh"
#include "app.hh"
#include "colors.hh"
#include "controls.hh"
#include "game.hh"

#ifndef NDEBUG
    #include "test.hh"
#endif

using namespace adt;

namespace frame
{

static void mainLoop();

Pair<f32, f32> g_unit; /* draw size unit */

f32 g_uiWidth = 192.0f * 0.50f;
f32 g_uiHeight; /* set in prepareDraw */

f64 g_dt = 0.0;
f64 g_gameTime = 0.0;

f64 g_currDrawTime = 0.0;
f64 g_frameTime = 0.0;
f64 g_lastFrameTime = 0.0;

f64 g_prevTime = 0.0;
int g_nfps = 0;

Ubo g_uboProjView;

static void
updateDrawTime()
{
    g_currDrawTime = utils::timeNowMS();
    g_frameTime = g_currDrawTime - g_lastFrameTime;
    g_lastFrameTime = g_currDrawTime;
}

void
run()
{
    WindowSetCursorImage(app::g_pWindow, "default");

    g_prevTime = utils::timeNowS();

    WindowBindGlContext(app::g_pWindow);
    WindowShowWindow(app::g_pWindow);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, app::g_pWindow);

    glPointSize(2.0f);
    glLineWidth(2.0f);
#endif

    /*glEnable(GL_CULL_FACE);*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    math::V4 col;
    col.xyz = colors::get(colors::IDX::BLACK);
    col.a = 1.0f;

    glClearColor(col.r, col.g, col.b, col.a);

    UboCreateBuffer(&g_uboProjView, sizeof(math::M4)*2, GL_DYNAMIC_DRAW);

    updateDrawTime(); /* reset delta time before drawing */
    updateDrawTime();

    WindowSetSwapInterval(app::g_pWindow, 1);
    // WindowSetFullscreen(app::g_pWindow);

#ifndef NDEBUG
    test::math();
    test::locks();
#endif

    game::loadAssets();
    game::loadLevel();

    /* proc once to get events */
    WindowSwapBuffers(app::g_pWindow);
    controls::procMouse();
    controls::procKeys();
    WindowProcEvents(app::g_pWindow);

    WindowEnableRelativeMode(app::g_pWindow);

    mainLoop();
}

static void
mainLoop()
{
    Arena arena(SIZE_1K * 20);
    defer( freeAll(&arena) );

    g_gameTime = 0.0;
    g_dt = game::FIXED_DELTA_TIME;

    f64 currentTime = utils::timeNowS();
    f64 accumulator = 0.0;

    while (app::g_pWindow->bRunning || app::g_pMixer->bRunning)
    {
        f64 newTime = utils::timeNowS();
        f64 frameTime = newTime - currentTime;
        currentTime = newTime;
        if (frameTime > 0.25) frameTime = 0.25;

        accumulator += frameTime;

        WindowProcEvents(app::g_pWindow);
        updateDrawTime();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app::g_pWindow->wWidth, app::g_pWindow->wHeight);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        controls::g_camera.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);
        UboBufferData(&g_uboProjView, &controls::g_camera, 0, sizeof(math::M4) * 2);

        controls::procKeys();

        while (accumulator >= g_dt)
        {
            game::updateState();
            g_gameTime += g_dt;
            accumulator -= g_dt;
        }

        const f64 alpha = accumulator / g_dt;

        game::draw(&arena, alpha);

        ArenaReset(&arena);

        WindowSwapBuffers(app::g_pWindow);
        g_nfps++;
    }

    audio::MixerDestroy(app::g_pMixer);

#ifndef NDEBUG
    UboDestroy(&g_uboProjView);
    game::cleanup();
#endif
}

} /* namespace frame */
