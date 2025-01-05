#pragma once

#include "adt/math.hh"
#include "controls.hh"

using namespace adt;

namespace keybinds
{

enum class ARG_TYPE : u8 { NONE, VOID_, LONG_, F32_, F64_, U64_, U64_BOOL_, BOOL_, VEC2_, VEC3_, VEC4_ };

struct Arg
{
    ARG_TYPE eType {};
    union {
        null nil;
        void* p;
        long l;
        f32 f;
        f64 d;
        u64 u;
        struct { u64 u; bool b; } ub;
        bool b;
        math::V2 v2;
        math::V3 v3;
        math::V4 v4;
    } uVal {};
};

union PFn
{
    void* ptr;
    void (*none)();
    void (*void_)(void*);
    void (*long_)(long);
    void (*f32_)(f32);
    void (*f64_)(f64);
    void (*u64_)(u64);
    void (*u64b)(u64, bool);
    void (*bool_)(bool);
    void (*v2)(math::V2);
    void (*v3)(math::V3);
    void (*v4)(math::V4);
};

struct Command
{
    bool bRepeat;
    u8 key;
    PFn pfn;
    Arg arg;

    void exec() const;
};

inline const Command inl_aCommands[] {
    {true,  KEY_A,     (void*)controls::move,              {ARG_TYPE::VEC2_, {.v2 {-1.0f, 0.0f}}}},
    {true,  KEY_D,     (void*)controls::move,              {ARG_TYPE::VEC2_, {.v2 {1.0f, 0.0f}}} },
    {false, KEY_P,     (void*)controls::togglePause,       {ARG_TYPE::NONE}                     },
    {false, KEY_Q,     (void*)controls::toggleMouseLock,   {ARG_TYPE::NONE}                     },
    {false, KEY_ESC,   (void*)controls::quit,              {ARG_TYPE::NONE}                     },
    {false, KEY_F,     (void*)controls::toggleFullscreen,  {ARG_TYPE::NONE}                     },
    {false, KEY_V,     (void*)controls::toggleVSync,       {ARG_TYPE::NONE}                     },
    {false, KEY_SPACE, (void*)controls::releaseBall,       {ARG_TYPE::NONE}                     },
    {false, KEY_J,     (void*)controls::toggleDebugScreen, {ARG_TYPE::NONE}                     },
    {false, KEY_B,     (void*)controls::toggleStepDebug,   {ARG_TYPE::NONE}                     },
};

inline const Command inl_aModCommands[] {
    {true, KEY_LEFTSHIFT, (void*)controls::mulDirection, {ARG_TYPE::F32_, {.f = 2.0f}}},
    {true, KEY_LEFTALT,   (void*)controls::mulDirection, {ARG_TYPE::F32_, {.f = 0.5f}}},
};

inline void
Command::exec() const
{
    switch (arg.eType)
    {
        case ARG_TYPE::NONE:
        pfn.none();
        break;

        case ARG_TYPE::VOID_:
        pfn.void_(arg.uVal.p);
        break;

        case ARG_TYPE::LONG_:
        pfn.long_(arg.uVal.l);
        break;

        case ARG_TYPE::F32_:
        pfn.f32_(arg.uVal.f);
        break;

        case ARG_TYPE::F64_:
        pfn.f64_(arg.uVal.d);
        break;

        case ARG_TYPE::U64_:
        pfn.u64_(arg.uVal.u);
        break;

        case ARG_TYPE::U64_BOOL_:
        pfn.u64b(arg.uVal.ub.u, arg.uVal.ub.b);
        break;

        case ARG_TYPE::BOOL_:
        pfn.bool_(arg.uVal.b);
        break;

        case ARG_TYPE::VEC2_:
        pfn.v2(arg.uVal.v2);
        break;

        case ARG_TYPE::VEC3_:
        pfn.v3(arg.uVal.v3);
        break;

        case ARG_TYPE::VEC4_:
        pfn.v4(arg.uVal.v4);
        break;
    }
}

} /* namespace keybinds */
