#include "frame.hh"

#include "adt/FixedAllocator.hh"
#include "adt/defer.hh"
#include "app.hh"
#include "colors.hh"
#include "controls.hh"
#include "game.hh"
#include "test.hh"

using namespace adt;

namespace frame
{

static void mainLoop();

Pair<f32, f32> g_unit; /* draw size unit */

f32 g_uiWidth = 192.0f * 0.50f;
f32 g_uiHeight; /* set in prepareDraw */

long g_currTime = 0;
f64 g_deltaTime = 0.0;
long g_lastTime = 0;

f64 g_currDrawTime = 0.0;
f64 g_frameTime = 0.0;
f64 g_lastFrameTime = 0.0;

f64 g_prevTime = 0.0;
int g_nfps = 0;

Ubo g_uboProjView;

static Pair<f32, f32> s_aspect(16.0f, 9.0f);

static u8 s_aMemGame[SIZE_8K / 2] {};
static u8 s_aMemDraw[SIZE_1M / 2] {};

static void
updateDeltaTime()
{
    g_currTime = utils::timeNowUS();
    g_deltaTime = (g_currTime - g_lastTime) / 1000000.0;
    g_lastTime = g_currTime;
}

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

    updateDeltaTime(); /* reset delta time before drawing */
    updateDeltaTime();
    updateDrawTime();
    updateDrawTime();

    WindowSetSwapInterval(app::g_pWindow, 1);
    WindowSetFullscreen(app::g_pWindow);

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

static int
gameStateLoop([[maybe_unused]] void* pNull)
{
    FixedAllocator alFrame(s_aMemGame, sizeof(s_aMemGame));

    while (app::g_pWindow->bRunning || app::g_pMixer->bRunning)
    {
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
    FixedAllocator alloc(s_aMemDraw, sizeof(s_aMemDraw));

    thrd_t thrdUpdateGameState {};
    thrd_create(&thrdUpdateGameState, gameStateLoop, nullptr);
    defer( thrd_join(thrdUpdateGameState, nullptr) );

    while (app::g_pWindow->bRunning || app::g_pMixer->bRunning)
    {
        WindowProcEvents(app::g_pWindow);
        updateDrawTime();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app::g_pWindow->wWidth, app::g_pWindow->wHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UboBufferData(&g_uboProjView, &controls::g_camera, 0, sizeof(math::M4) * 2);

        game::draw(&alloc.base);

        FixedAllocatorReset(&alloc);
        WindowSwapBuffers(app::g_pWindow);
        g_nfps++;
    }

#ifndef NDEBUG
    UboDestroy(&g_uboProjView);
    game::cleanup();
#endif
}

} /* namespace frame */
