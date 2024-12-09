
#include <stdio.h>
#include <SDL2/SDL.h>
#include <sys/mman.h>

#include "filter.h"

#define WIDTH 3840
#define HEIGHT 2160
#define FPS 30.0f


int _width = WIDTH;
int _height = HEIGHT;
float _fps = FPS;
SDL_Renderer *_renderer;
FILTER _filter[16];
int _colors[16];
int _filterNum = 0;
u_int8_t *_backbuffer;
SDL_Surface *_backsurface;
SDL_AudioDeviceID _adevice;
SDL_AudioDeviceID _mdevice;
SDL_AudioSpec _aspec;
SDL_AudioSpec _mspec;
SDL_AudioSpec _receivedaudiospec;
float _asample[2048];
float _msample[2048];
int aframecount = 0;
int _recordingdevicecount = 0;

double vframe = 0;
double aframe = (1.0/44100.0)*1024.0; // 0.02322
double aelapsed = 0.0;
int framecount=0;


#include <sys/time.h>
double currenttime() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    double  seconds = ((double)te.tv_sec) + ((double)te.tv_usec / 1000.0); // calculate milliseconds
    return seconds;
}

void acallback(void *userdata, uint8_t * stream, int len)
{
	int i;

	SDL_zero(_asample);
	
	for(i = 0; i < _filterNum; i++)
	{
		filter_audio(&_filter[i], _asample, framecount, aelapsed);
	}
	for(int i = 0; i < 2048; i++) 
		((float*)stream)[i] = i%2 ? _asample[i/2] : _asample[1024+i/2];
	aelapsed += aframe;
}

void mcallback(void *userdata, uint8_t * stream, int len)
{
	int i;

	SDL_zero(_msample);
	
	for(int i = 0; i < 2048; i++) 
	{
		if(i%2)
		{
			_msample[i/2] = ((float*)stream)[i];
		}
		else
		{
			_msample[1024+i/2] = ((float*)stream)[i];
		}
	}	
	for(i = 0; i < _filterNum; i++)
	{
		filter_mic(&_filter[i], _msample, framecount, aelapsed);
	}
        aelapsed += aframe;
}

CRESULT video(char* argv[], int64_t framecount)
{
	SDL_Texture *screentexture;
	int x, y, i;

	SDL_RenderClear(_renderer);

	for(i = 0; i < _filterNum; i++)
	{
		if(filter_video(&_filter[i], _backsurface->pixels, _width, _height, _colors[i], argv[6+(i*3)], framecount) == CFAILED)
			return CFAILED;
	}
	screentexture = SDL_CreateTextureFromSurface(_renderer, _backsurface);
	SDL_RenderCopy(_renderer, screentexture, NULL, NULL);
	SDL_DestroyTexture(screentexture);

	SDL_RenderPresent(_renderer);
	
	return CSUCCESS;
}


int main(int argc, char* argv[])
{
	SDL_Window* window = NULL;
	SDL_Event event;
	int keypress = 0;
	
	double velapsed = 0.0;
	double xelapsed = 0.0;
	double vgoal;
	double agoal;
	double now, lasttime;
	Uint32 rmask, gmask, bmask, amask;
	int numAudioDrivers, i;
	int mindex;
	
	if(argc < 6)
	{
		printf("Usage: %s [WIDTH] [HEIGHT] [FPS] [filter1.so] [COLOR1] [ARGUMENT1] [filter2.so] [COLOR2] [ARGUMENT2]...\r\n", argv[0]);
		return 1;
	}
	
	_width = atoi(argv[1]);
	_height = atoi(argv[2]);
	_fps = (float)atoi(argv[3]);

	vframe = 1.0/_fps;

	_filterNum = (argc-4)/3;

	for(i = 0; i < _filterNum; i++)
	{
		_colors[i] = (int)strtol(argv[5+(i*3)], NULL, 16);
		if(filter_create(&_filter[i], argv[4+(i*3)], _fps) == CFAILED)
		{
			printf("Error: opening filter failed\r\n");
			return 1;
		}
	}
	_backbuffer = (Uint8*)malloc(_width * _height * 4);

	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
	{
		printf("Error: SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

        _recordingdevicecount = SDL_GetNumAudioDevices(SDL_TRUE);

	if(_recordingdevicecount < 1)
	{
		printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}

	for(int i = 0; i < _recordingdevicecount; ++i)
	{
		const char* deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);
		printf("%d - %s\n", i, deviceName);
	}

        printf("Choose audio: \n");
	scanf("%d", &mindex);


	window = SDL_CreateWindow( "AV-Viewer 1.0 | AGPL-3.0, Dirk Jan Buter, https://dirkjanbuter.com/", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, SDL_WINDOW_SHOWN);
	if(!window)
	{
		printf("Error creating window: %s\r\n", SDL_GetError());
		return 1;
	}

	_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!_renderer)
	{
		printf("Error creating renderer: %s\r\n", SDL_GetError());
		return 1;
	}

	if(window == NULL)
	{
		printf("Error: Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

	//Get window surface
	
	 
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0x00ff0000;
    gmask = 0x0000ff00;
    bmask = 0x000000ff;
    amask = 0xff000000;
#else
    rmask = 0x0000ff00;
    gmask = 0x00ff0000;
    bmask = 0xff000000;
    amask = 0x000000ff;
#endif
	_backsurface = SDL_CreateRGBSurface(0, _width, _height, 32, rmask, gmask, bmask, amask);

	mlock(_backsurface->pixels, _backsurface->pitch * _backsurface->h);
	mlock(_backbuffer, _width * _height * 4);
	mlock(_asample, sizeof(_asample));
	mlock(_msample, sizeof(_msample));

	vgoal = 0.0;
	agoal = 0.0;
		
	
	now = currenttime();
	if(video(argv, framecount++) == CFAILED)
		return 1;
	velapsed += vframe;
	
	// Sound
	SDL_zero(_aspec);
	_aspec.freq = 44100;
	_aspec.silence = 0;
	_aspec.format = AUDIO_F32;
	_aspec.channels = 2;
	_aspec.samples = 1024;
	_aspec.callback = acallback;
	_adevice = SDL_OpenAudioDevice(NULL, 0, &_aspec, NULL, 0);
	SDL_PauseAudioDevice(_adevice, 0);
	
	// Mic
	SDL_zero(_mspec);
	_mspec.freq = 44100;
	_mspec.silence = 0;
	_mspec.format = AUDIO_F32;
	_mspec.channels = 2;
	_mspec.samples = 1024;
	_mspec.callback = mcallback;
	_mdevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(mindex, SDL_TRUE), SDL_TRUE, &_mspec, &_receivedaudiospec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	SDL_PauseAudioDevice(_mdevice, SDL_FALSE);

        if(1)
        {
		while(!quit)
		{
			while(SDL_PollEvent(&event))
			{
		      		if(event.type == SDL_KEYDOWN)
		      		{
		      		      for(i = 0; i < _filterNum; i++)
	                              {
		                            if(filter_video(&_filter[i], event.key.keysym.scancode) == CFAILED)
			                    {
			                          quit = 1;
			                    }
	                              }
		      		}
		      		   		
		      		switch(event.type)
		      		{
			 	      case SDL_QUIT:
			     	            quit = 1;
			     		    break;
		      		}
		 	}
		 	
		 	if(velapsed <= aelapsed)
			{		
				if(video(argv, framecount++) == CFAILED)
					break;
				velapsed += vframe;
			}
		}
	}
	
	munlock(_msample, sizeof(_msample));
	munlock(_asample, sizeof(_asample));
	munlock(_backbuffer, _width * _height * 4);
	munlock(_backsurface->pixels, _backsurface->pitch * _backsurface->h);

    	SDL_CloseAudioDevice(_mdevice);
    	SDL_CloseAudioDevice(_adevice);


	SDL_FreeSurface(_backsurface);
	SDL_Quit();

	for(i = 0; i < _filterNum; i++)
	{
		filter_destroy(&_filter[i]);
	}
	free(_backbuffer);
	
	return 0;
}



