///////////////////////////////////////////////////////////
//
// AudioEngine.cpp: Core audio functionality
// 
//////////////////////////////////////////////////////////////////////////////
// DevStudio!namespace Audio DevStudio!class AudioEngine
#include "AudioEngine.h"
#include "portaudio.h"
#include <v8.h>
using namespace v8;

//////////////////////////////////////////////////////////////////////////////
/*! Initalize */
Audio::AudioEngine::AudioEngine( Persistent<Function>& audioCallback ) :
	m_audioCallback(audioCallback),
	m_uNumInputChannels(2),
	m_uNumOutputChannels(2) {

	// Initialize our audio core
	Pa_Initialize();

	PaStream *stream;
	PaError err;

	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream( &stream,
								0,          /* no input channels */
								2,          /* stereo output */
								paFloat32,  /* 32 bit floating point output */
								SAMPLE_RATE,
								256,        /* frames per buffer, i.e. the number
													of sample frames that PortAudio will
													request from the callback. Many apps
													may want to use
													paFramesPerBufferUnspecified, which
													tells PortAudio to pick the best,
													possibly changing, buffer size.*/
								audioCallbackSource, /* this is your callback function */
								&data ); /*This is a pointer that will be passed to
													your callback*/
} // end AudioEngine::AudioEngine()
