#pragma once

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define COL_NORM  "\x1B[0m"
#define COL_RED  "\x1B[31m"
#define COL_GREEN  "\x1B[32m"
#define COL_YELLOW  "\x1B[33m"
#define COL_BLUE  "\x1B[34m"
#define COL_MAGENTA  "\x1B[35m"
#define COL_CYAN  "\x1B[36m"
#define COL_WHITE  "\x1B[37m"

#define COUT(...) fprintf(stdout, __VA_ARGS__)
#define CERR(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
    #define DCOUT(...) COUT(__VA_ARGS__)
    #define DCERR(...) CERR(__VA_ARGS__)
#else
    #define DCOUT(...)
    #define DCERR(...)
#endif

enum _LOG_SEV
{
    _LOG_SEV_OK,
    _LOG_SEV_GOOD,
    _LOG_SEV_WARN,
    _LOG_SEV_BAD,
    _LOG_SEV_FATAL,
    _LOG_SEV_ENUM_SIZE
};

inline const char* _LOG_SEV_STR[] = {
    "",
    COL_GREEN "GOOD: " COL_NORM,
    COL_YELLOW "WARNING: " COL_NORM,
    COL_RED "BAD: " COL_NORM,
    COL_RED "FATAL: " COL_NORM
};

#if defined __clang__ || __GNUC__
    #define ADT_FILE __FILE_NAME__
#else
    #define ADT_FILE __FILE__
#endif

#define _LOG(SEV, ...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        assert(SEV >= 0 && SEV < _LOG_SEV_ENUM_SIZE && "wrong _LOG_SEV*");                                             \
        CERR("(%s%s, %d): ", _LOG_SEV_STR[SEV], ADT_FILE, __LINE__);                                                   \
        CERR(__VA_ARGS__);                                                                                             \
        switch (SEV)                                                                                                   \
        {                                                                                                              \
            default:                                                                                                   \
                break;                                                                                                 \
            case _LOG_SEV_BAD:                                                                                         \
                exit(static_cast<int>(SEV));                                                                           \
            case _LOG_SEV_FATAL:                                                                                       \
                abort();                                                                                               \
        }                                                                                                              \
    } while (0)

#define LOG_OK(...) _LOG(_LOG_SEV_OK, __VA_ARGS__)
#define LOG_GOOD(...) _LOG(_LOG_SEV_GOOD, __VA_ARGS__)
#define LOG_WARN(...) _LOG(_LOG_SEV_WARN, __VA_ARGS__)
#define LOG_BAD(...) _LOG(_LOG_SEV_BAD, __VA_ARGS__)
#define LOG_FATAL(...) _LOG(_LOG_SEV_FATAL, __VA_ARGS__)

/* TODO: */
// #ifdef _WIN32
//     #define ASSERT assert
// #else
//     #define ASSERT(EXPR, ...)
// #endif
