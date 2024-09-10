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

f32 g_fov = 90.0f;
f32 g_uiWidth = 192.0f * 0.75;
f32 g_uiHeight; /* set in prepareDraw */

f64 g_currTime = 0.0;
f64 g_deltaTime = 0.0;
f64 g_lastFrameTime = 0.0;

f64 g_prevTime;
int g_fpsCount = 0;

Ubo g_uboProjView;

static Pair<f32, f32> s_aspect(16.0f, 9.0f);

static u8 s_aFrameMem[SIZE_8K] {};

static void
updateDeltaTime()
{
    g_currTime = utils::timeNowMS();
    g_deltaTime = g_currTime - g_lastFrameTime;
    g_lastFrameTime = g_currTime;
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

    game::loadThings();

    updateDeltaTime(); /* reset delta time before drawing */
    updateDeltaTime();

    /* proc once to get events */
    WindowSwapBuffers(app::g_pApp);
    controls::procMouse();
    controls::procKeys();
    WindowProcEvents(app::g_pApp);

    WindowEnableRelativeMode(app::g_pApp);

    mainLoop();
}

template<typename T>
inline math::V2
nextPos(const T& e, bool bNormalizeDir)
{
    auto dir = bNormalizeDir ? math::normalize(e.dir) : e.dir;
    return e.base.pos + (dir * (g_deltaTime * e.speed));
}

static void
mainLoop()
{
    game::loadLevel();

    FixedAllocator alFrame (s_aFrameMem, sizeof(s_aFrameMem));

    while (app::g_pApp->bRunning || app::g_pMixer->bRunning) /* wait for mixer to stop also */
    {
        WindowProcEvents(app::g_pApp);
        updateDeltaTime();
        controls::procKeys();

        controls::g_camera.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app::g_pApp->wWidth, app::g_pApp->wHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UboBufferData(&g_uboProjView, &controls::g_camera, 0, sizeof(math::M4) * 2);

        game::updateGame();
        game::drawEntities();
        game::drawFPSCounter(&alFrame.base);

        FixedAllocatorReset(&alFrame);
        WindowSwapBuffers(app::g_pApp);
        g_fpsCount++;
    }

#ifndef NDEBUG
    UboDestroy(&g_uboProjView);
    game::cleanup();
#endif
}

} /* namespace frame */
