#define _POSIX_C_SOURCE 199309L

#include "Client.hh"

#include "adt/Arena.hh"
#include "adt/Vec.hh"
#include "adt/logs.hh"
#include "input.hh"

#include <string.h>


namespace platform
{
namespace wayland
{

EGLint eglLastErrorCode = EGL_SUCCESS;

#ifdef DEBUG
#    define EGLD(C)                                                                                                    \
        {                                                                                                              \
            C;                                                                                                         \
            if ((eglLastErrorCode = eglGetError()) != EGL_SUCCESS)                                                     \
                LOG_FATAL("eglLastErrorCode: %#x\n", eglLastErrorCode);                                                \
        }
#else
#    define EGLD(C) C
#endif

static const zwp_relative_pointer_v1_listener relativePointerListener {
	.relative_motion = input::relativePointerMotionHandler
};

static const wl_pointer_listener pointerListener {
    .enter = input::pointerEnterHandler,
    .leave = input::pointerLeaveHandler,
    .motion = input::pointerMotionHandler,
    .button = input::pointerButtonHandler,
    .axis = input::pointerAxisHandler,
    .frame {},
    .axis_source {},
    .axis_stop {},
    .axis_discrete {},
    .axis_value120 {},
    .axis_relative_direction {}
};

static const wl_keyboard_listener keyboardListener {
    .keymap = input::keyboardKeymapHandler,
    .enter = input::keyboardEnterHandler,
    .leave = input::keyboardLeaveHandler,
    .key = input::keyboardKeyHandler,
    .modifiers = input::keyboardModifiersHandler,
    .repeat_info = input::keyboardRepeatInfoHandler
};

/* mutter compositor will complain if we do not pong */
static void
xdgWmBasePing(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_wm_base* xdgWmBase,
    [[maybe_unused]] u32 serial
)
{
    auto app = (Client*)data;
    xdg_wm_base_pong(app->xdgWmBase, serial);
}

static void
seatCapabilitiesHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_seat* seat,
    [[maybe_unused]] u32 capabilities
)
{
    auto app = (Client*)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        app->pointer = wl_seat_get_pointer(app->seat);
        app->cursorTheme = wl_cursor_theme_load(nullptr, 24, app->shm);
        wl_pointer_add_listener(app->pointer, &pointerListener, app);
        LOG_OK("pointer works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        app->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->keyboard, &keyboardListener, app);
        LOG_OK("keyboard works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH)
    {
        //
        LOG_OK("touch works.\n");
    }
}

static const wl_seat_listener seatListener {
    .capabilities = seatCapabilitiesHandler,
	.name = {}
};

static void
xdgSurfaceConfigureHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_surface* xdgSurface,
    [[maybe_unused]] u32 serial
)
{
    auto app = (Client*)data;
    xdg_surface_ack_configure(xdgSurface, serial);
    app->base.bConfigured = true;
}

static void
xdgToplevelConfigureHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_toplevel* xdgToplevel,
    [[maybe_unused]] s32 width,
    [[maybe_unused]] s32 height,
    [[maybe_unused]] wl_array* states
)
{
    auto app = (Client*)data;

    if (width > 0 && height > 0)
    {
        if (width != app->base.wWidth || height != app->base.wHeight)
        {
            wl_egl_window_resize(app->eglWindow, width, height, 0, 0);
            app->base.wWidth = width;
            app->base.wHeight = height;
        }
    }
}

static void
xdgToplevelCloseHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_toplevel* xdgToplevel
)
{
    auto app = (Client*)data;
    app->base.bRunning = false;
    LOG_OK("closing...\n");
}

static void
xdgToplevelConfigureBounds(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_toplevel* xdgToplevel,
    [[maybe_unused]] s32 width,
    [[maybe_unused]] s32 height
)
{
    //
}

static const xdg_surface_listener xdgSurfaceListener {
    .configure = xdgSurfaceConfigureHandler
};

static const xdg_wm_base_listener xdgWmBaseListener {
    .ping = xdgWmBasePing
};


static void
xdgToplevelWmCapabilities(
    [[maybe_unused]] void* data,
    [[maybe_unused]] xdg_toplevel* xdgToplevel,
    [[maybe_unused]] wl_array* capabilities
)
{
    //
}

static const xdg_toplevel_listener xdgToplevelListener {
    .configure = xdgToplevelConfigureHandler,
    .close = xdgToplevelCloseHandler,
    .configure_bounds = xdgToplevelConfigureBounds,
    .wm_capabilities = xdgToplevelWmCapabilities
};

static void
registryGlobalHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_registry* registry,
    [[maybe_unused]] u32 name,
    [[maybe_unused]] const char* interface,
    [[maybe_unused]] u32 version
)
{
    LOG_OK("interface: '%s', version: %u, name: %u\n", interface, version, name);
    auto app = (Client*)data;

    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        app->seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(app->seat, &seatListener, app);
    }
    else if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        app->compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        app->xdgWmBase = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(app->xdgWmBase, &xdgWmBaseListener, app);
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        app->pointerConstraints = (zwp_pointer_constraints_v1*)wl_registry_bind(
            registry, name, &zwp_pointer_constraints_v1_interface, version
        );
    }
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        app->relativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(
            registry, name, &zwp_relative_pointer_manager_v1_interface, version
        );
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        app->shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        app->output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
    }
    /*else if (strcmp(interface, wp_tearing_control_manager_v1_interface.name) == 0)*/
    /*{*/
    /*    app->tearingConrolManager = (wp_tearing_control_manager_v1*)wl_registry_bind(registry, name, &wp_tearing_control_manager_v1_interface, version);*/
    /*    app->tearingConrol = wp_tearing_control_manager_v1_get_tearing_control(app->tearingConrolManager, app->surface);*/
    /*}*/
}

static void
registryGlobalRemoveHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_registry* wlRegistry,
    [[maybe_unused]] u32 name
)
{
    /* can be empty */
}

static const wl_registry_listener registryListener {
    .global = registryGlobalHandler,
    .global_remove = registryGlobalRemoveHandler
};

Client::Client(String name)
{
    static WindowInterface vTable {
        .init = (decltype(WindowInterface::init))ClientInit,
        .disableRelativeMode = (decltype(WindowInterface::disableRelativeMode))ClientDisableRelativeMode,
        .enableRelativeMode = (decltype(WindowInterface::enableRelativeMode))ClientEnableRelativeMode,
        .togglePointerRelativeMode = (decltype(WindowInterface::togglePointerRelativeMode))ClientTogglePointerRelativeMode,
        .toggleFullscreen = (decltype(WindowInterface::toggleFullscreen))ClientToggleFullscreen,
        .hideCursor = (decltype(WindowInterface::hideCursor))ClientHideCursor,
        .setCursorImage = (decltype(WindowInterface::setCursorImage))ClientSetCursorImage,
        .setFullscreen = (decltype(WindowInterface::setFullscreen))ClientSetFullscreen,
        .unsetFullscreen = (decltype(WindowInterface::unsetFullscreen))ClientUnsetFullscreen,
        .bindGlContext = (decltype(WindowInterface::bindGlContext))ClientBindGlContext,
        .unbindGlContext = (decltype(WindowInterface::unbindGlContext))ClientUnbindGlContext,
        .setSwapInterval = (decltype(WindowInterface::setSwapInterval))ClientSetSwapInterval,
        .toggleVSync = (decltype(WindowInterface::toggleVSync))ClientToggleVSync,
        .swapBuffers = (decltype(WindowInterface::swapBuffers))ClientSwapBuffers,
        .procEvents = (decltype(WindowInterface::procEvents))ClientProcEvents,
        .showWindow = (decltype(WindowInterface::showWindow))ClientShowWindow,
        .destroy = (decltype(WindowInterface::destroy))ClientDestroy,
    };

    base = {};
    base.pVTable = {&vTable};

    base.sName = name;
}

void
ClientDestroy(Client* s)
{
    LOG_OK("cleanup ...\n");

    if (s->base.bPointerRelativeMode) ClientDisableRelativeMode(s);
    if (s->pointer) wl_pointer_destroy(s->pointer);
    if (s->cursorTheme) wl_cursor_theme_destroy(s->cursorTheme);
    if (s->keyboard) wl_keyboard_destroy(s->keyboard);
    if (s->xdgSurface) xdg_surface_destroy(s->xdgSurface);
    if (s->xdgToplevel) xdg_toplevel_destroy(s->xdgToplevel);
    if (s->surface) wl_surface_destroy(s->surface);
    if (s->shm) wl_shm_destroy(s->shm);
    if (s->xdgWmBase) xdg_wm_base_destroy(s->xdgWmBase);
    if (s->pointerConstraints) zwp_pointer_constraints_v1_destroy(s->pointerConstraints);
    if (s->relativePointerManager) zwp_relative_pointer_manager_v1_destroy(s->relativePointerManager);
    if (s->cursorSurface) wl_surface_destroy(s->cursorSurface);
    if (s->seat) wl_seat_destroy(s->seat);
    if (s->output) wl_output_destroy(s->output);
    if (s->compositor) wl_compositor_destroy(s->compositor);
    if (s->registry) wl_registry_destroy(s->registry);
}

void
ClientInit(Client* s)
{
    Arena arena(SIZE_8K);

    if ((s->display = wl_display_connect(nullptr)))
        LOG_OK("wayland display connected\n");
    else
        LOG_OK("error connecting wayland display\n");

    s->registry = wl_display_get_registry(s->display);
    wl_registry_add_listener(s->registry, &registryListener, s);
    wl_display_dispatch(s->display);
    wl_display_roundtrip(s->display);

    if (s->compositor == nullptr || s->xdgWmBase == nullptr)
        LOG_FATAL("no wl_shm, wl_compositor or xdg_wm_base support\n");

    EGLD( s->eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(s->display)) );
    if (s->eglDisplay == EGL_NO_DISPLAY)
        LOG_FATAL("failed to create EGL display\n");

    EGLint major, minor;
    if (!eglInitialize(s->eglDisplay, &major, &minor))
        LOG_FATAL("failed to initialize EGL\n");
    EGLD();

    /* Default is GLES */
    if (!eglBindAPI(EGL_OPENGL_API))
        LOG_FATAL("eglBindAPI(EGL_OPENGL_API) failed\n");

    LOG_OK("egl: major: %d, minor: %d\n", major, minor);

    EGLint count;
    EGLD( eglGetConfigs(s->eglDisplay, nullptr, 0, &count) );

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        // EGL_ALPHA_SIZE, 8, /* KDE makes window transparent even in fullscreen */
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        // EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        // EGL_MIN_SWAP_INTERVAL, 0,
        // EGL_MAX_SWAP_INTERVAL, 1,
        // EGL_SAMPLE_BUFFERS, 1,
        // EGL_SAMPLES, 4,
        EGL_NONE
    };

    EGLint n = 0;
    Vec<EGLConfig> configs(&arena.base, count);
    VecSetSize(&configs, count);
    EGLD( eglChooseConfig(s->eglDisplay, configAttribs, VecData(&configs), count, &n) );
    if (n == 0)
        LOG_FATAL("Failed to choose an EGL config\n");

    EGLConfig eglConfig = configs[0];

    EGLint contextAttribs[] {
        // EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#ifdef DEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
        EGL_NONE,
    };

    EGLD( s->eglContext = eglCreateContext(s->eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs) );

    s->surface = wl_compositor_create_surface(s->compositor);
    s->xdgSurface = xdg_wm_base_get_xdg_surface(s->xdgWmBase, s->surface);
    s->xdgToplevel = xdg_surface_get_toplevel(s->xdgSurface);

    xdg_toplevel_set_title(s->xdgToplevel, s->base.sName.pData);
    xdg_toplevel_set_app_id(s->xdgToplevel, s->base.sName.pData);

    xdg_surface_add_listener(s->xdgSurface, &xdgSurfaceListener, s);
    xdg_toplevel_add_listener(s->xdgToplevel, &xdgToplevelListener, s);

    s->eglWindow = wl_egl_window_create(s->surface, s->base.wWidth, s->base.wHeight);
    EGLD( s->eglSurface = eglCreateWindowSurface(s->eglDisplay, eglConfig, (EGLNativeWindowType)(s->eglWindow), nullptr) );

    wl_surface_commit(s->surface);
    wl_display_roundtrip(s->display);

    ArenaFreeAll(&arena);

    s->base.bRunning = true;
}

void
ClientEnableRelativeMode(Client* s)
{
    if (s->base.bPointerRelativeMode) return;

    s->base.bPointerRelativeMode = true;

    ClientHideCursor(s);

    if (s->cursorSurface) wl_surface_destroy(s->cursorSurface);

    s->lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        s->pointerConstraints, s->surface, s->pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT
    );

    s->relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(s->relativePointerManager, s->pointer);
    zwp_relative_pointer_v1_add_listener(s->relativePointer, &relativePointerListener, s);
}

void
ClientDisableRelativeMode(Client* s)
{
    if (!s->base.bPointerRelativeMode) return;

    s->base.bPointerRelativeMode = false;

    zwp_locked_pointer_v1_destroy(s->lockedPointer);
    zwp_relative_pointer_v1_destroy(s->relativePointer);

    ClientSetCursorImage(s, "default");
}

void
ClientHideCursor(Client* s)
{
    s->base.bHideCursor = true;
    wl_pointer_set_cursor(s->pointer, s->_pointerSerial, nullptr, 0, 0);
}

void
ClientSetCursorImage(Client* s, String cursorType)
{
    wl_cursor* cursor = wl_cursor_theme_get_cursor(s->cursorTheme, cursorType.pData);
    if (!cursor)
    {
        LOG_WARN("failed to set cursor to '%s', falling back to 'default'\n", cursorType.pData);
        cursor = wl_cursor_theme_get_cursor(s->cursorTheme, "default");
    }

    if (cursor)
    {
        s->cursorImage = cursor->images[0];
        wl_buffer* cursorBuffer = wl_cursor_image_get_buffer(s->cursorImage);

        s->cursorSurface = wl_compositor_create_surface(s->compositor);
        wl_pointer_set_cursor(s->pointer, s->_pointerSerial, s->cursorSurface, 0, 0);
        wl_surface_attach(s->cursorSurface, cursorBuffer, 0, 0);
        wl_surface_commit(s->cursorSurface);

        wl_pointer_set_cursor(
            s->pointer,
            s->_pointerSerial,
            s->cursorSurface,
            s->cursorImage->hotspot_x,
            s->cursorImage->hotspot_y
        );
    }
}

void 
ClientSetFullscreen(Client* s)
{
    if (s->base.bFullscreen) return;

    s->base.bFullscreen = true;
    xdg_toplevel_set_fullscreen(s->xdgToplevel, s->output);
}

void
ClientUnsetFullscreen(Client* s)
{
    if (!s->base.bFullscreen) return;

    s->base.bFullscreen = false;
    xdg_toplevel_unset_fullscreen(s->xdgToplevel);
}

void
ClientTogglePointerRelativeMode(Client* s)
{
    s->base.bPointerRelativeMode ? ClientDisableRelativeMode(s) : ClientEnableRelativeMode(s);
    LOG_OK("relative mode: %d\n", s->base.bPointerRelativeMode);
}

void
ClientToggleFullscreen(Client* s)
{
    s->base.bFullscreen ? ClientUnsetFullscreen(s) : ClientSetFullscreen(s);
    LOG_OK("fullscreen mode: %d\n", s->base.bFullscreen);
}

void 
ClientBindGlContext(Client* s)
{
    EGLD ( eglMakeCurrent(s->eglDisplay, s->eglSurface, s->eglSurface, s->eglContext) );
}

void 
ClientUnbindGlContext(Client* s)
{
    EGLD( eglMakeCurrent(s->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
ClientSetSwapInterval(Client* s, int interval)
{
    s->base.swapInterval = interval;
    EGLD( eglSwapInterval(s->eglDisplay, interval) );
}

void
ClientToggleVSync(Client* s)
{
    s->base.swapInterval = !s->base.swapInterval;
    EGLD( eglSwapInterval(s->eglDisplay, s->base.swapInterval) );
    LOG_OK("swapInterval: %d\n", s->base.swapInterval);

    /*auto hint = s->base.swapInterval == 0 ? WP_TEARING_CONTROL_V1_PRESENTATION_HINT_VSYNC : WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC;*/
    /*wp_tearing_control_v1_set_presentation_hint(s->tearingConrol, hint);*/
}

void
ClientSwapBuffers(Client* s)
{
    EGLD( eglSwapBuffers(s->eglDisplay, s->eglSurface) );
    if (wl_display_dispatch(s->display) == -1)
        LOG_FATAL("wl_display_dispatch error\n");
}

void 
ClientProcEvents([[maybe_unused]] Client* s)
{
    //
}

void
ClientShowWindow([[maybe_unused]] Client* s)
{
    //
}

} /* namespace wayland */
} /* namespace platform */
