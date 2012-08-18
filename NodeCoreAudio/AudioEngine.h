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
#include <node_internals.h>
#include <node_object_wrap.h>
using namespace v8; using namespace std;

#define SAMPLE_RATE			(44100)
#define PA_SAMPLE_TYPE      paFloat32
#define FRAMES_PER_BUFFER   (256)

namespace Audio {

	//////////////////////////////////////////////////////////////////////////
	//! Core audio functionality
	class AudioEngine : public node::ObjectWrap {
	public:
		AudioEngine( Local<Function>& audioCallback ); //!< Initalize

		static void Init(v8::Handle<v8::Object> target);	//!< Our node.js instantiation function
		static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);	//!< Instantiate from instance factory
		
		//! Our static audio callback function. This is static, and we pass a pointer to ourselves in as userData,
		//! allowing us to call a non-static function on the correct instance of AudioEngine.
		static int audioCallbackSource( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, 
								 const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData ) {
			return ((AudioEngine*)userData)->audioCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
		} // end audioCallbackSource()


		//!< Our main audio callback
		int audioCallback( const void *input, void *output, unsigned long uSampleFrames, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags );

		~AudioEngine();	//!< Destructor

	private:
		static v8::Persistent<v8::Function> constructor;				
		static v8::Handle<v8::Value> New( const v8::Arguments& args );		//!< Our V8 new operator
		static v8::Handle<v8::Value> IsActive( const v8::Arguments& args );	//!< Returns whether the audio stream active
		static v8::Handle<v8::Value> ProcessIfNewData( const v8::Arguments& args );	//!< Hands audio to a javascript callback if we have new data

		void copyBuffer( int uSampleFrames, float* sourceBuffer, float* destBuffer, bool bZeroSource = false ); //!< Copy one buffer into another
		void wrapObject( v8::Handle<v8::Object> object ); //!< Wraps a handle into an object

		PaStream* m_pStream;	//!< Our audio stream

		int m_uNumInputChannels,
			m_uNumOutputChannels;
		
		Persistent<Function> m_audioCallback;		//!< We call this with audio data whenever we get it

		Handle<Value> m_uSampleFrames;				//!< Number of sample frames in this buffer (set in audioCallbackSource)

// 		v8::Value m_tempSampleIndex,			//!< Temp preallocated sample index variable
// 				  m_tempSample;					//!< A temporary preallocated sample we use to copy data inside audioCallback()

		bool m_bNewAudioData;		//!< Our new audio data flag

		v8::Local<v8::Array> m_inputBuffer,
							 m_outputBuffer;

		int m_uSleepTime;		//!< The number of milliseconds we put the audio thread to 
								//!< sleep when we're waiting for new data from the non-process thread

		vector<vector<float> > m_fInputProcessSamples,		//!< Samples coming from the sound card
							   m_fInputNonProcessSamples,	//!< Samples we can change from the main thread
							   m_fOutputProcessSamples,		//!< Samples going to the sound card
							   m_fOutputNonProcessSamples;	//!< Samples we can change from the main thread

	}; // end class AudioEngine
	
} // end namespace Audio

