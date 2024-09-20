#include "frame.hh"

#include "app.hh"
#include "colors.hh"
#include "controls.hh"
#include "game.hh"
#include "math.hh"

#include "adt/FixedAllocator.hh"

using namespace adt;

namespace frame
{

static void mainLoop();

Pair<f32, f32> g_unit; /* draw size unit */

f32 g_uiWidth = 192.0f * 0.75;
f32 g_uiHeight; /* set in prepareDraw */

f64 g_currTime = 0.0;
f64 g_deltaTime = 0.0;
f64 g_lastDeltaTime = 0.0;

f64 g_currDrawTime = 0.0;
f64 g_frameTime = 0.0;
f64 g_lastFrameTime = 0.0;

f64 g_prevTime = 0.0;
int g_fpsCount = 0;

Ubo g_uboProjView;

static Pair<f32, f32> s_aspect(16.0f, 9.0f);

static u8 s_aFrameMemGame[SIZE_8K] {};
static u8 s_aFrameMemDraw[SIZE_8K] {};

inline void
updateDeltaTime()
{
    g_currTime = utils::timeNowMS();
    g_deltaTime = g_currTime - g_lastDeltaTime;
    g_lastDeltaTime = g_currTime;
}

inline void
updateDrawTime()
{
    g_currDrawTime = utils::timeNowMS();
    g_frameTime = g_currDrawTime - g_lastFrameTime;
    g_lastFrameTime = g_currDrawTime;
}

void
run()
{
    WindowSetCursorImage(app::g_pApp, "default");

    g_prevTime = utils::timeNowS();

    WindowBindGlContext(app::g_pApp);
    WindowShowWindow(app::g_pApp);

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, app::g_pApp);
#endif

    /*glEnable(GL_CULL_FACE);*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    math::V4 col = {colors::get(colors::IDX::BLACK), 1.0f};
    glClearColor(col.r, col.g, col.b, col.a);

    UboCreateBuffer(&g_uboProjView, sizeof(math::M4)*2, GL_DYNAMIC_DRAW);

    updateDeltaTime(); /* reset delta time before drawing */
    updateDeltaTime();
    updateDrawTime();
    updateDrawTime();

    game::loadAssets();
    game::loadLevel();

    /* proc once to get events */
    WindowSwapBuffers(app::g_pApp);
    controls::procMouse();
    controls::procKeys();
    WindowProcEvents(app::g_pApp);

    WindowEnableRelativeMode(app::g_pApp);

    mainLoop();
}

static int
gameStateLoop([[maybe_unused]] void* pArg)
{
    FixedAllocator alFrame (s_aFrameMemGame, sizeof(s_aFrameMemGame));

    while (app::g_pApp->bRunning || app::g_pMixer->bRunning)
    {
        WindowProcEvents(app::g_pApp);
        updateDeltaTime();
        controls::procKeys();

        controls::g_camera.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);

        game::updateState();

        FixedAllocatorReset(&alFrame);

        utils::sleepMS(game::SLEEP_TIME_MS);
    }

    return thrd_success;
}

static void
mainLoop()
{
    FixedAllocator alFrame (s_aFrameMemDraw, sizeof(s_aFrameMemGame));

    thrd_t thrdUpdatePhysics;
    thrd_create(&thrdUpdatePhysics, gameStateLoop, nullptr);

    while (app::g_pApp->bRunning || app::g_pMixer->bRunning)
    {
        updateDrawTime();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app::g_pApp->wWidth, app::g_pApp->wHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UboBufferData(&g_uboProjView, &controls::g_camera, 0, sizeof(math::M4) * 2);

        game::drawEntities();
        game::drawFPSCounter(&alFrame.base);

        FixedAllocatorReset(&alFrame);
        WindowSwapBuffers(app::g_pApp);
        g_fpsCount++;
    }

    thrd_join(thrdUpdatePhysics, nullptr);

#ifndef NDEBUG
    UboDestroy(&g_uboProjView);
    game::cleanup();
#endif
}

} /* namespace frame */
