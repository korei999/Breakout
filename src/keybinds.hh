#pragma once

#include "adt/math.hh"
#include "controls.hh"

using namespace adt;

namespace keybinds
{

enum class ARG_TYPE : u8 { NONE, LONG, F32, F64, U64, U64_BOOL, BOOL, VEC2, VEC3, VEC4 };

struct Arg
{
    ARG_TYPE eType {};
    union {
        null nil;
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

struct ModCommand
{
    bool bRepeat;
    MOD_STATE eMod;
    PFn pfn;
    Arg arg;

    void exec() const;
};

inline const Command inl_aCommands[] {
    {true,  KEY_A,     (void*)controls::move,              {ARG_TYPE::VEC2, {.v4 = {-1.0f, 0.0f}}}},
    {true,  KEY_D,     (void*)controls::move,              {ARG_TYPE::VEC2, {.v4 = {1.0f, 0.0f}}} },
    {false, KEY_P,     (void*)controls::togglePause,       {ARG_TYPE::NONE}                       },
    {false, KEY_Q,     (void*)controls::toggleMouseLock,   {ARG_TYPE::NONE}                       },
    {false, KEY_ESC,   (void*)controls::quit,              {ARG_TYPE::NONE}                       },
    {false, KEY_F,     (void*)controls::toggleFullscreen,  {ARG_TYPE::NONE}                       },
    {false, KEY_V,     (void*)controls::toggleVSync,       {ARG_TYPE::NONE}                       },
    {false, KEY_SPACE, (void*)controls::releaseBall,       {ARG_TYPE::NONE}                       },
    {false, KEY_J,     (void*)controls::toggleDebugScreen, {ARG_TYPE::NONE}                       },
    {false, KEY_B,     (void*)controls::toggleStepDebug,   {ARG_TYPE::NONE}                       },
};

inline const ModCommand inl_aModCommands[] {
    {true, MOD_STATE::SHIFT, (void*)controls::mulDirection, {ARG_TYPE::F32, {.f = 2.0f}}},
    {true, MOD_STATE::ALT,   (void*)controls::mulDirection, {ARG_TYPE::F32, {.f = 0.5f}}},
};

inline void
execCommand(PFn pfn, Arg arg)
{
    switch (arg.eType)
    {
        case ARG_TYPE::NONE:
        pfn.none();
        break;

        case ARG_TYPE::LONG:
        pfn.long_(arg.uVal.l);
        break;

        case ARG_TYPE::F32:
        pfn.f32_(arg.uVal.f);
        break;

        case ARG_TYPE::F64:
        pfn.f64_(arg.uVal.d);
        break;

        case ARG_TYPE::U64:
        pfn.u64_(arg.uVal.u);
        break;

        case ARG_TYPE::U64_BOOL:
        pfn.u64b(arg.uVal.ub.u, arg.uVal.ub.b);
        break;

        case ARG_TYPE::BOOL:
        pfn.bool_(arg.uVal.b);
        break;

        case ARG_TYPE::VEC2:
        pfn.v2(arg.uVal.v2);
        break;

        case ARG_TYPE::VEC3:
        pfn.v3(arg.uVal.v3);
        break;

        case ARG_TYPE::VEC4:
        pfn.v4(arg.uVal.v4);
        break;
    }
}

inline void
Command::exec() const
{
    execCommand(pfn, arg);
}

inline void
ModCommand::exec() const
{
    execCommand(pfn, arg);
}

} /* namespace keybinds */
