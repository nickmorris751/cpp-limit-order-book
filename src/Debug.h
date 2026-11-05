#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef MATCHING_ENGINE_LOG
#define ENGINE_LOG(...) printf(__VA_ARGS__)
#else
#define ENGINE_LOG(...) ((void)0)
#endif


#endif