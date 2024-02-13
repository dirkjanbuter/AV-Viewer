#include "filter.h"

#ifdef WIN32

CRESULT filter_create(FILTER *v, char *filename, int fps)
{
    v->handle = LoadLibrary(filename);
    if (!v->handle) {
        return CFAILED;
    }

    v->filtercreate = (FILTER_FUN_CREATE)GetProcAddress(v->handle, "filtercreate");
    v->filterdestroy = (FILTER_FUN_DESTROY)GetProcAddress(v->handle, "filterdestroy");
    v->filtervideo = (FILTER_FUN_VIDEO)GetProcAddress(v->handle, "filtervideo");
    v->filteraudio = (FILTER_FUN_AUDIO)GetProcAddress(v->handle, "filteraudio");

    if(!v->filtervideo)	
	    v->filtervideo = (FILTER_FUN_VIDEO)GetProcAddress(v->handle, "filterstep");
	

    if (!v->filtervideo) {
        return CFAILED;
    }

    if(v->filtercreate)
        if((*v->filtercreate)(fps) == 0)
            return CFAILED;

    return CSUCCESS;
}

void filter_destroy(FILTER *v)
{
    if(v->filterdestroy)
        (*v->filterdestroy)();

    FreeLibrary(v->handle);
}

CRESULT filter_step(FILTER *v, IMGBUFFER *buffer, int w, int h, unsigned int color, char *text, int64_t framecount) {
    if(w != buffer->w || h != buffer->h)
        return CFAILED;


     if((*v->filtervideo)(buffer->data, w, h, color, text, framecount) == 0)
        return CFAILED;

    return CSUCCESS;
}

#else

CRESULT filter_create(FILTER *v, char *filename, int fps)
{
    char *error;

    v->handle = dlopen (filename, RTLD_LAZY);
    if (!v->handle) {
        fputs (dlerror(), stderr);
        return CFAILED;
    }

    v->filtercreate = dlsym(v->handle, "filtercreate");
    v->filterdestroy = dlsym(v->handle, "filterdestroy");
    v->filteraudio = dlsym(v->handle, "filteraudio");


    v->filtervideo = dlsym(v->handle, "filtervideo");
    if(!v->filtervideo) 
	    v->filtervideo = dlsym(v->handle, "filterstep");
    
    
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        return CFAILED;
    }

    if(v->filtercreate)
        if((*v->filtercreate)(fps) == 0)
            return CFAILED;

    return CSUCCESS;
}

void filter_destroy(FILTER *v)
{
    if(v->filterdestroy)
        (*v->filterdestroy)();

    dlclose(v->handle);
}

CRESULT filter_video(FILTER *v, u_int8_t *buffer, int w, int h, unsigned int color, char *text, int64_t framecount)
{
    if((*v->filtervideo)(buffer, w, h, color, text, framecount) == 0)
        return CFAILED;
    return CSUCCESS;
}

CRESULT filter_audio(FILTER *v, float *frame, int64_t framecount, float elapsed)
{
    if(!v->filteraudio)
    	return CFAILED;	
    if((*v->filteraudio)(frame, framecount, elapsed) == 0)
        return CFAILED;
    return CSUCCESS;
}


#endif





