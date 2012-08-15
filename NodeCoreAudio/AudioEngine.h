///////////////////////////////////////////////////////////
//
// AudioEngine.h: Core audio functionality
// Copyright (c) 2010 - iZotope, Inc.  All Rights Reserved
//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "portaudio.h"
#include <v8.h>
#include <vector>
using namespace v8; using namespace std;

#define SAMPLE_RATE (44100)

namespace Audio {


	typedef struct {
		float left_phase;
		float right_phase;
	} paTestData;

	static paTestData data;

	/* This routine will be called by the PortAudio engine when audio is needed.
	 It may called at interrupt level on some machines so don't do anything
	 that could mess up the system like calling malloc() or free().
	*/ 
	static int patestCallback( const void *inputBuffer, void *outputBuffer,
							   unsigned long framesPerBuffer,
							   const PaStreamCallbackTimeInfo* timeInfo,
							   PaStreamCallbackFlags statusFlags,
							   void *userData )
	{
		

		/* Cast data passed through stream to our structure. */
// 		paTestData *data = (paTestData*)userData; 
// 		float *out = (float*)outputBuffer;
// 		unsigned int i;
// 		(void) inputBuffer; /* Prevent unused variable warning. */
//     
// 		for( i=0; i<framesPerBuffer; i++ )
// 		{
// 			*out++ = data->left_phase;  /* left */
// 			*out++ = data->right_phase;  /* right */
// 			/* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
// 			data->left_phase += 0.01f;
// 			/* When signal reaches top, drop back down. */
// 			if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
// 			/* higher pitch so we can distinguish left and right. */
// 			data->right_phase += 0.03f;
// 			if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
// 		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//! Core audio functionality
	class AudioEngine {
	public:
		AudioEngine( Persistent<Function>& audioCallback ); //!< Initalize
		
		int audioCallbackSource( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, 
								 const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData ) {
			
// 			paTestData *data = (paTestData*)userData; 
// 			float *out = (float*)outputBuffer;
// 			unsigned int i;
// 			(void) inputBuffer; /* Prevent unused variable warning. */
// 
// 			for( i=0; i<framesPerBuffer; i++ )
// 			{
// 				*out++ = data->left_phase;  /* left */
// 				*out++ = data->right_phase;  /* right */
// 				/* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
// 				data->left_phase += 0.01f;
// 				/* When signal reaches top, drop back down. */
// 				if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
// 				/* higher pitch so we can distinguish left and right. */
// 				data->right_phase += 0.03f;
// 				if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
// 			}
			std::vector<float> teste;
	
			m_uSampleFrames= Number::New( framesPerBuffer );
			m_inputBuffer= Number::New( teste );
			//m_outputBuffer= Array::New( outputBuffer );

			m_audioCallback->Call( m_uSampleFrames, m_inputBuffer, m_outputBuffer );
			
			return 0;
		}
	private:
		int m_uNumInputChannels,
			m_uNumOutputChannels;
		
		Persistent<Function> m_audioCallback;		//!< We call this with audio data whenever we get it
		Handle<Value> m_uSampleFrames;				//!< Number of sample frames in this buffer (set in audioCallbackSource)
		Handle<Value> m_inputBuffer,
					  m_outputBuffer;

	}; // end class AudioEngine
	
} // end namespace Audio

