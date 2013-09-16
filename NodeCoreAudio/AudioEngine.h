//////////////////////////////////////////////////////////////////////////////
//
// AudioEngine.cpp: Core audio functionality
// 
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "portaudio.h"
#include <v8.h>
#include <vector>
#include <node_internals.h>
#include <node_object_wrap.h>
using namespace v8; using namespace std;

#define DEFAULT_SAMPLE_RATE         (44100)
#define DEFAULT_SAMPLE_FORMAT      paFloat32
#define DEFAULT_FRAMES_PER_BUFFER  (256)

namespace Audio {

	//////////////////////////////////////////////////////////////////////////
	//! Core audio functionality
    class AudioEngine : public node::ObjectWrap {
    public:

        AudioEngine( Local<Function>& callback, Local<Object> options, bool withThread );

		//! Initialize our node object
        static void Init(v8::Handle<v8::Object> target);
		//! Create a new instance of the audio engine
        static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);

		Isolate* GetIsolate() { return m_pIsolate; }
		Locker* GetLocker() { return m_pLocker; }

        //! Run the main blocking audio loop
        static void* RunAudioLoop( void* data );

        ~AudioEngine();		//!< OOL Destructor

    private:
        static v8::Persistent<v8::Function> constructor;                
        static v8::Handle<v8::Value> New( const v8::Arguments& args );	//!< Create a v8 object
        
		//! Returns whether the PortAudio stream is active
        static v8::Handle<v8::Value> isActive( const v8::Arguments& args );
		//! Get the name of an audio device with a given ID number
        static v8::Handle<v8::Value> getDeviceName( const v8::Arguments& args );
		//! Get the number of available devices
        static v8::Handle<v8::Value> getNumDevices( const v8::Arguments& args );

		//! Closes and reopens the PortAudio stream
        void restartStream();

		static v8::Handle<v8::Value> write( const v8::Arguments& args );		//!< Write samples to the current audio device
		static v8::Handle<v8::Value> read( const v8::Arguments& args );			//!< Read samples from the current audio device

		static v8::Handle<v8::Value> setOptions( const v8::Arguments& args );	//!< Set options, restarts audio stream
		static v8::Handle<v8::Value> getOptions( const v8::Arguments& args );	//!< Gets options

		static void processAudioCallback( uv_work_t* handle, int status );		//!< Runs one round of audio processing on the audio thread	
		static void afterWork(uv_work_t* handle, int status) {};

		void applyOptions( Local<Object> options );				//!< Sets the given options and restarts the audio stream if necessary
		void wrapObject( v8::Handle<v8::Object> object );		//!< Wraps a handle into an object

        void queueOutputBuffer( Handle<Array> result );			//!< Queues up an array to be sent to the sound card
        void setSample( int position, Handle<Value> sample );	//!< Sets a sample in the queued output buffer

		Handle<Array> getInputBuffer();							//!< Returns a v8 array filled with input samples
		Handle<Number> getSample( int position );				//!< Returns a sound card sample converted to a v8 Number

        PaStream *m_pPaStream;				//!< The PortAudio stream object
        PaStreamParameters m_inputParams,	//!< PortAudio stream parameters
                           m_outputParams;

		Handle<Array> m_hInputBuffer;		//!< Our pre-allocated input buffer

		uv_thread_t ptStreamThread,			//!< Our stream thread
					jsAudioThread;			//!< Our JavaScript Audio thread

        uv_mutex_t m_mutex;					//!< A mutex for transferring data between the DSP and UI threads

        Persistent<Function> audioJsCallback;		//!< We call this with audio data whenever we get it

        int m_uOutputChannels,		//!< Number of input channels
            m_uInputChannels,		//!< Number of output channels
            m_uInputDevice,			//!< Index of the current input device
            m_uOutputDevice,		//!< Index of the current output device
            m_uSampleRate,			//!< Current sample rate
            m_uSamplesPerBuffer,	//!< Number of sample frames per process buffers
            m_uSampleFormat,		//!< Index of the current sample format
            m_uSampleSize;			//!< Number of bytes per sample frame

        unsigned int m_uNumCachedOutputSamples;		//!< Number of samples we've queued up, outgoing to the sound card

        bool m_bInputOverflowed,			//!< Set when our buffers have overflowed
			 m_bOutputUnderflowed,
			 m_bInterleaved;				//!< Set when we're processing interleaved buffers

        char* m_cachedInputSampleBlock,		//!< Temp buffer to hold buffer results
            * m_cachedOutputSampleBlock;

		Isolate* m_pIsolate;

		Locker* m_pLocker;

    }; // end class AudioEngine
    
} // end namespace Audio

