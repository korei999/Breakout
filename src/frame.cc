#include "frame.hh"

#include "adt/Arena.hh"
#include "adt/defer.hh"
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
    app::g_pWindow->setCursorImage("default");

    g_prevTime = utils::timeNowS();

    app::g_pWindow->bindGlContext();
    app::g_pWindow->showWindow();

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

    g_uboProjView.createBuffer(sizeof(math::M4)*2, GL_DYNAMIC_DRAW);

    updateDrawTime(); /* reset delta time before drawing */
    updateDrawTime();

    app::g_pWindow->setSwapInterval(1);

#ifndef NDEBUG
    test::math();
    test::locks();
#endif

    game::loadAssets();
    game::loadLevel();

    /* proc once to get events */
    app::g_pWindow->swapBuffers();
    controls::procMouse();
    controls::procKeys();
    app::g_pWindow->procEvents();

#ifdef NDEBUG
    app::g_pWindow->setFullscreen();
#endif
    app::g_pWindow->disableRelativeMode();

    mainLoop();
}

static void
mainLoop()
{
    Arena arena(SIZE_1K * 20);
    defer( arena.freeAll() );

    g_gameTime = 0.0;
    g_dt = game::FIXED_DELTA_TIME;

    f64 currentTime = utils::timeNowS();
    f64 accumulator = 0.0;

    auto& win = *app::g_pWindow;
    auto& mix = *app::g_pMixer;

    while (win.m_bRunning || mix.m_bRunning)
    {
        if (!win.m_bPaused)
        {
            f64 newTime = utils::timeNowS();
            f64 frameTime = newTime - currentTime;
            currentTime = newTime;
            if (frameTime > 0.25) frameTime = 0.25;

            accumulator += frameTime;
        }

        win.procEvents();
        updateDrawTime();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, win.m_wWidth, win.m_wHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        controls::g_camera.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);
        g_uboProjView.bufferData(&controls::g_camera, 0, sizeof(math::M4) * 2);

        controls::procKeys();

        if (controls::g_bStepDebug && !win.m_bPaused)
        {
            game::updateState(&arena);
            g_gameTime += g_dt;
            accumulator -= g_dt;
        }
        else
        {
            while (accumulator >= g_dt && !win.m_bPaused)
            {
                game::updateState(&arena);
                g_gameTime += g_dt;
                accumulator -= g_dt;
            }
        }

        const f64 alpha = accumulator / g_dt;

        game::draw(&arena, alpha);

        arena.reset();

        app::g_pWindow->swapBuffers();
        g_nfps++;
    }

    app::g_pMixer->destroy();

#ifndef NDEBUG
    g_uboProjView.destroy();
    game::cleanup();
#endif
}

} /* namespace frame */
