#ifndef FILTER_H_INCLUDED
#define FILTER_H_INCLUDED

#ifdef WIN32
#include <signal.h>
#include <unistd.h>
#else
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "log.h"

#ifdef WIN32

typedef int (*FILTER_FUN_CREATE)(int);
typedef int (*FILTER_FUN_DESTROY)();
typedef int (*FILTER_FUN_VIDEO)(unsigned char *, int, int, unsigned int, char *, int64_t);
typedef int (*FILTER_FUN_AUDIO)(float *, int, float);


typedef struct FILTER {
    HINSTANCE  handle;
    FILTER_FUN_CREATE filtercreate;
    FILTER_FUN_DESTROY filterdestroy;
    FILTER_FUN_VIDEO filtervideo;
    FILTER_FUN_AUDIO filteraudio;
} FILTER;
#else
typedef struct FILTER {
    void *handle;
    int (*filtercreate)(int);
    int (*filterdestroy)();
    int (*filtervideo)(unsigned char *, int, int, unsigned int, char *, int64_t);
    int (*filteraudio)(float *, int64_t, float);
} FILTER;
#endif


CRESULT filter_create(FILTER *v, char *filename, int fps);
void filter_destroy(FILTER *v);
CRESULT filter_video(FILTER *v, u_int8_t *buffer, int w, int h, unsigned int color, char *text, int64_t framecount);
CRESULT filter_audio(FILTER *v, float *frame, int64_t framecount, float elapsed);

#endif

