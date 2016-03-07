//////////////////////////////////////////////////////////////////////////////
//
// AudioEngine.cpp: Core audio functionality
//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "portaudio.h"
#include <v8.h>
#include <vector>
#include <node_object_wrap.h>
#include <nan.h>
#include <uv.h>
#include <node.h>
#include <stdlib.h>

#ifdef __linux__
	#include <unistd.h>
	#include <string.h>
#endif

#define DEBUG 1

using namespace v8;
using namespace std;

#define AUDIO_ENGINE_CLASSNAME     "AudioEngine"

#define DEFAULT_SAMPLE_RATE        (44100)
#define DEFAULT_SAMPLE_FORMAT      paFloat32
#define DEFAULT_FRAMES_PER_BUFFER  (256)
#define DEFAULT_NUM_BUFFERS        (8)

#define VALIDATE_PROPERTY(options, name, type, out) ( \
	out = (Nan::HasOwnProperty(options, Nan::New<String>(name).ToLocalChecked()).FromMaybe(false) ? \
		Nan::To<type>(Nan::Get(options, Nan::New<String>(name).ToLocalChecked()).ToLocalChecked()).FromJust() : out ))

namespace Audio {

	//////////////////////////////////////////////////////////////////////////
	//! Core audio functionality
	class AudioEngine : public Nan::ObjectWrap {
	public:

		AudioEngine(Local<Object> options);

		//! Initialize our node object
		static void Init(v8::Handle<v8::Object> target);
		//! Create a new instance of the audio engine
		//static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);
        static NAN_METHOD(NewInstance);

		//! Run the main blocking audio loop
		void RunAudioLoop();

		~AudioEngine();		//!< OOL Destructor

	private:
		static void initializePortAudio();

		void setDefaultOptions();
		void initializeStream();
		void initializeInputBuffer();
		void reinitializeStream();

		void printOptions();

		static v8::Persistent<v8::Function> constructor;
		//static v8::Handle<v8::Value> New( const v8::Arguments& args );	//!< Create a v8 object
        static NAN_METHOD(New);

		//! Returns whether the PortAudio stream is active
		//static v8::Handle<v8::Value> isActive( const v8::Arguments& args );
        static NAN_METHOD(isActive);
		//! Get the name of an audio device with a given ID number
		//static v8::Handle<v8::Value> getDeviceName( const v8::Arguments& args );
        static NAN_METHOD(getDeviceName);
		//! Get the number of available devices
		//static v8::Handle<v8::Value> getNumDevices( const v8::Arguments& args );
        static NAN_METHOD(getNumDevices);

		//! Closes and reopens the PortAudio stream
		void restartStream();

		//static v8::Handle<v8::Value> write( const v8::Arguments& args );		//!< Write samples to the current audio device
        static NAN_METHOD(write);
		//static v8::Handle<v8::Value> read( const v8::Arguments& args );			//!< Read samples from the current audio device
        static NAN_METHOD(read);
		//static v8::Handle<v8::Value> isBufferEmpty( const v8::Arguments& args );	//!< Returns whether the data buffer is empty
        static NAN_METHOD(isBufferEmpty);

		//static v8::Handle<v8::Value> setOptions( const v8::Arguments& args );	//!< Set options, restarts audio stream
        static NAN_METHOD(setOptions);
		//static v8::Handle<v8::Value> getOptions( const v8::Arguments& args );	//!< Gets options
        static NAN_METHOD(getOptions);

		static void afterWork(uv_work_t* handle, int status) {};

		void applyOptions(Local<Object> options);				//!< Sets the given options and restarts the audio stream if necessary
		void wrapObject(v8::Handle<v8::Object> object);		//!< Wraps a handle into an object

		void queueOutputBuffer( Handle<Array> result );			//!< Queues up an array to be sent to the sound card
		void setSample( int position, Handle<Value> sample );	//!< Sets a sample in the queued output buffer

		Local<Array> getInputBuffer();							//!< Returns a v8 array filled with input samples
		Handle<Number> getSample( int position );				//!< Returns a sound card sample converted to a v8 Number

		PaStream *stream;				//!< The PortAudio stream object
		PaStreamParameters streamInputParams,	//!< PortAudio stream parameters
						   streamOutputParams;

		Local<Array> inputBuffer;		//!< Our pre-allocated input buffer

		uv_thread_t streamThread;			//!< Our stream thread

		uv_mutex_t m_mutex;					//!< A mutex for transferring data between the DSP and UI threads

		int  outputChannels,     // Number of input channels
			 inputChannels,      // Number of output channels
			 inputDevice,        // Index of the current input device
			 outputDevice,       // Index of the current output device
			 sampleRate,         // Current sample rate
			 samplesPerBuffer,   // Number of sample frames per process buffers
			 numBuffers,         // Number of sample buffers to keep ahead
			 sampleFormat,       // Index of the current sample format
			 sampleSize;         // Number of bytes per sample frame
		bool inputOverflowed,    // Set when our buffers have overflowed
			 outputUnderflowed,
			 readMicrophone,     // TODO: document me !
			 interleaved;        // Set when we're processing interleaved buffers

		unsigned int currentWriteBuffer, currentReadBuffer;
		unsigned int* numCachedOutputSamples;		//!< Number of samples we've queued up, outgoing to the sound card

		char* cachedInputSampleBlock,		//!< Temp buffer to hold buffer results
			** cachedOutputSampleBlock,
			* cachedOutputSampleBlockForWriting;

	};

}
