#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "Client.hh"

#include "adt/Arena.hh"
#include "adt/Vec.hh"
#include "adt/defer.hh"
#include "input.hh"
#include "adt/logs.hh"

#include <string.h>


namespace platform
{
namespace wayland
{

EGLint eglLastErrorCode = EGL_SUCCESS;

#ifndef NDEBUG
#    define EGLD(C)                                                                                                    \
        {                                                                                                              \
            C;                                                                                                         \
            if ((eglLastErrorCode = eglGetError()) != EGL_SUCCESS)                                                     \
                LOG_FATAL("eglLastErrorCode: {:#x}\n", eglLastErrorCode);                                              \
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
    xdg_wm_base_pong(app->m_xdgWmBase, serial);
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
        app->m_pointer = wl_seat_get_pointer(app->m_seat);
        app->m_cursorTheme = wl_cursor_theme_load(nullptr, 24, app->m_shm);
        wl_pointer_add_listener(app->m_pointer, &pointerListener, app);
        LOG_OK("pointer works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        app->m_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->m_keyboard, &keyboardListener, app);
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
    app->m_bConfigured = true;
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
        if (width != app->m_wWidth || height != app->m_wHeight)
        {
            wl_egl_window_resize(app->m_eglWindow, width, height, 0, 0);
            app->m_wWidth = width;
            app->m_wHeight = height;
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
    app->m_bRunning = false;
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
    LOG_OK("interface: '{}', version: {}, name: {}\n", interface, version, name);
    auto app = (Client*)data;

    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        app->m_seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(app->m_seat, &seatListener, app);
    }
    else if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        app->m_compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        app->m_xdgWmBase = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(app->m_xdgWmBase, &xdgWmBaseListener, app);
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        app->m_pointerConstraints = (zwp_pointer_constraints_v1*)wl_registry_bind(
            registry, name, &zwp_pointer_constraints_v1_interface, version
        );
    }
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        app->m_relativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(
            registry, name, &zwp_relative_pointer_manager_v1_interface, version
        );
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        app->m_shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        app->m_output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
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

void
Client::destroy()
{
    LOG_OK("cleanup ...\n");

    if (m_bPointerRelativeMode) disableRelativeMode();
    if (m_eglDisplay) eglTerminate(m_eglDisplay);
    if (m_pointer) wl_pointer_destroy(m_pointer);
    if (m_cursorTheme) wl_cursor_theme_destroy(m_cursorTheme);
    if (m_keyboard) wl_keyboard_destroy(m_keyboard);
    if (m_xdgSurface) xdg_surface_destroy(m_xdgSurface);
    if (m_xdgToplevel) xdg_toplevel_destroy(m_xdgToplevel);
    if (m_surface) wl_surface_destroy(m_surface);
    if (m_shm) wl_shm_destroy(m_shm);
    if (m_xdgWmBase) xdg_wm_base_destroy(m_xdgWmBase);
    if (m_pointerConstraints) zwp_pointer_constraints_v1_destroy(m_pointerConstraints);
    if (m_relativePointerManager) zwp_relative_pointer_manager_v1_destroy(m_relativePointerManager);
    if (m_cursorSurface) wl_surface_destroy(m_cursorSurface);
    if (m_seat) wl_seat_destroy(m_seat);
    if (m_output) wl_output_destroy(m_output);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry) wl_registry_destroy(m_registry);
}

void
Client::start()
{
    Arena arena(SIZE_1K);
    defer( arena.freeAll() );

    if ((m_display = wl_display_connect(nullptr)))
        LOG_OK("wayland display connected\n");
    else LOG_OK("error connecting wayland display\n");

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registryListener, this);
    wl_display_dispatch(m_display);
    wl_display_roundtrip(m_display);

    if (m_compositor == nullptr || m_xdgWmBase == nullptr)
        LOG_FATAL("no wl_shm, wl_compositor or xdg_wm_base support\n");

    EGLD( m_eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(m_display)) );
    if (m_eglDisplay == EGL_NO_DISPLAY)
        LOG_FATAL("failed to create EGL display\n");

    EGLint major, minor;
    if (!eglInitialize(m_eglDisplay, &major, &minor))
        LOG_FATAL("failed to initialize EGL\n");
    EGLD();

    /* Default is GLES */
    if (!eglBindAPI(EGL_OPENGL_API))
        LOG_FATAL("eglBindAPI(EGL_OPENGL_API) failed\n");

    LOG_OK("egl: major: {}, minor: {}\n", major, minor);

    EGLint count;
    EGLD( eglGetConfigs(m_eglDisplay, nullptr, 0, &count) );

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        // EGL_ALPHA_SIZE, 8, /* KDE makes window transparent even in fullscreen */
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        // EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        // EGL_MIN_SWAP_INTERVAL, 0,
        // EGL_MAX_SWAP_INTERVAL, 1,
        // EGL_SAMPLE_BUFFERS, 1,
        // EGL_SAMPLES, 4,
        EGL_NONE
    };

    EGLint n = 0;
    Vec<EGLConfig> configs(&arena, count);
    configs.setSize(count);
    EGLD( eglChooseConfig(m_eglDisplay, configAttribs, configs.data(), count, &n) );
    if (n == 0)
        LOG_FATAL("Failed to choose an EGL config\n");

    EGLConfig eglConfig = configs[0];

    EGLint contextAttribs[] {
        // EGL_CONTEXT_CLIENT_VERSION, 3,
        // EGL_CONTEXT_MAJOR_VERSION, 3,
        // EGL_CONTEXT_MINOR_VERSION, 3,
        // EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#ifndef NDEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
        EGL_NONE,
    };

    EGLD( m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs) );

    m_surface = wl_compositor_create_surface(m_compositor);
    m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWmBase, m_surface);
    m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);

    xdg_toplevel_set_title(m_xdgToplevel, m_sName.data());
    xdg_toplevel_set_app_id(m_xdgToplevel, m_sName.data());

    xdg_surface_add_listener(m_xdgSurface, &xdgSurfaceListener, this);
    xdg_toplevel_add_listener(m_xdgToplevel, &xdgToplevelListener, this);

    m_eglWindow = wl_egl_window_create(m_surface, m_wWidth, m_wHeight);
    EGLD( m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, (EGLNativeWindowType)(m_eglWindow), nullptr) );

    wl_surface_commit(m_surface);
    wl_display_roundtrip(m_display);

    m_bRunning = true;
}

void
Client::enableRelativeMode()
{
    if (m_bPointerRelativeMode) return;

    m_bPointerRelativeMode = true;

    hideCursor();

    if (m_cursorSurface) wl_surface_destroy(m_cursorSurface);

    m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        m_pointerConstraints, m_surface, m_pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT
    );

    m_relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, m_pointer);
    zwp_relative_pointer_v1_add_listener(m_relativePointer, &relativePointerListener, this);
}

void
Client::disableRelativeMode()
{
    if (!m_bPointerRelativeMode) return;

    m_bPointerRelativeMode = false;

    zwp_locked_pointer_v1_destroy(m_lockedPointer);
    zwp_relative_pointer_v1_destroy(m_relativePointer);

    setCursorImage("default");
}

void
Client::hideCursor()
{
    m_bHideCursor = true;
    wl_pointer_set_cursor(m_pointer, m_pointerSerial, nullptr, 0, 0);
}

void
Client::setCursorImage(String cursorType)
{
    wl_cursor* cursor = wl_cursor_theme_get_cursor(m_cursorTheme, cursorType.data());
    if (!cursor)
    {
        LOG_WARN("failed to set cursor to '{}', falling back to 'default'\n", cursorType);
        cursor = wl_cursor_theme_get_cursor(m_cursorTheme, "default");
    }

    if (cursor)
    {
        m_cursorImage = cursor->images[0];
        wl_buffer* cursorBuffer = wl_cursor_image_get_buffer(m_cursorImage);

        m_cursorSurface = wl_compositor_create_surface(m_compositor);
        wl_pointer_set_cursor(m_pointer, m_pointerSerial, m_cursorSurface, 0, 0);
        wl_surface_attach(m_cursorSurface, cursorBuffer, 0, 0);
        wl_surface_commit(m_cursorSurface);

        wl_pointer_set_cursor(
            m_pointer,
            m_pointerSerial,
            m_cursorSurface,
            m_cursorImage->hotspot_x,
            m_cursorImage->hotspot_y
        );
    }
}

void 
Client::setFullscreen()
{
    if (m_bFullscreen) return;

    m_bFullscreen = true;
    xdg_toplevel_set_fullscreen(m_xdgToplevel, m_output);
}

void
Client::unsetFullscreen()
{
    if (!m_bFullscreen) return;

    m_bFullscreen = false;
    xdg_toplevel_unset_fullscreen(m_xdgToplevel);
}

void
Client::togglePointerRelativeMode()
{
    m_bPointerRelativeMode ? disableRelativeMode() : enableRelativeMode();
    LOG_OK("relative mode: {}\n", m_bPointerRelativeMode);
}

void
Client::toggleFullscreen()
{
    m_bFullscreen ? unsetFullscreen() : setFullscreen();
    LOG_OK("fullscreen mode: {}\n", m_bFullscreen);
}

void 
Client::bindGlContext()
{
    EGLD ( eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) );
}

void 
Client::unbindGlContext()
{
    EGLD( eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
Client::setSwapInterval(int interval)
{
    m_swapInterval = interval;
    EGLD( eglSwapInterval(m_eglDisplay, interval) );
}

void
Client::toggleVSync()
{
    m_swapInterval = !m_swapInterval;
    EGLD( eglSwapInterval(m_eglDisplay, m_swapInterval) );
    LOG_OK("swapInterval: {}\n", m_swapInterval);

    /*auto hint = base.swapInterval == 0 ? WP_TEARING_CONTROL_V1_PRESENTATION_HINT_VSYNC : WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC;*/
    /*wp_tearing_control_v1_set_presentation_hint(tearingConrol, hint);*/
}

void
Client::swapBuffers()
{
    EGLD( eglSwapBuffers(m_eglDisplay, m_eglSurface) );
}

void 
Client::procEvents()
{
    if (wl_display_dispatch(m_display) == -1)
    {
        CERR("wl_display_dispatch(): failed\n");
        exit(1);
    }
}

void
Client::showWindow()
{
    //
}

} /* namespace wayland */
} /* namespace platform */
