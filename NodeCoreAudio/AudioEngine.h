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

        ~AudioEngine();

    private:
        static v8::Persistent<v8::Function> constructor;                
        static v8::Handle<v8::Value> New( const v8::Arguments& args );
        
		//! Returns whether the PortAudio stream is active
        static v8::Handle<v8::Value> IsActive( const v8::Arguments& args );
		//! Get the name of an audio device with a given ID number
        static v8::Handle<v8::Value> GetDeviceName( const v8::Arguments& args );
		//! Get the number of available devices
        static v8::Handle<v8::Value> GetNumDevices( const v8::Arguments& args );

        static v8::Handle<v8::Value> Write( const v8::Arguments& args );		//!< Write samples to the current audio device
        static v8::Handle<v8::Value> Read( const v8::Arguments& args );			//!< Read samples from the current audio device

        static v8::Handle<v8::Value> SetOptions( const v8::Arguments& args );	//!< Set options, restarts audio stream
        static v8::Handle<v8::Value> GetOptions( const v8::Arguments& args );	//!< Gets options

        static void* streamThread(void *p);
		static void callCallback(uv_work_t* handle, int status);
        static void afterWork(uv_work_t* handle, int status) {};

        void applyOptions( Local<Object> options );

        void wrapObject( v8::Handle<v8::Object> object );
        void restartStream();

        void queueOutputBuffer(Handle<Array> result);
        Handle<Array> getInputBuffer();
        Handle<Number> getSample(int position);
        void setSample(int position, Handle<Value>);

        PaStream *m_pPaStream;				//!< The PortAudio stream object
        PaStreamParameters m_inputParams,	//!< PortAudio stream parameters
                           m_outputParams;

		Handle<Array> m_hInputBuffer;	//!< Our pre-allocated input buffer

		uv_thread_t ptStreamThread;		//!< Our stream thread
        uv_thread_t jsAudioThread;		//!< Our JavaScript Audio thread

        uv_mutex_t m_mutex;				//!< A mutex for transferring data between the DSP and UI threads

        Persistent<Function> audioJsCallback;   //!< We call this with audio data whenever we get it

        int m_uOutputChannels,		//!< Number of input channels
            m_uInputChannels,		//!< Number of output channels
            m_uInputDevice,			//!< Index of the current input device
            m_uOutputDevice,		//!< Index of the current output device
            m_uSampleRate,			//!< Current sample rate
            m_uSamplesPerBuffer,	//!< Number of sample frames per process buffers
            m_uSampleFormat,		//!< Index of the current sample format
            m_uSampleSize;			//!< Number of bytes per sample frame

        unsigned int m_uNumCachedOutputSamples;		//!< Number of samples we've queued up, outgoing to the sound card

        bool m_bInputOverflowed,	//!< Set when our buffers have overflowed
			 m_bOutputUnderflowed,
			 m_bInterleaved;		//!< Set when we're processing interleaved buffers

        char* m_cachedInputSampleBlock,		//!< Temp buffer to hold buffer results
            * m_cachedOutputSampleBlock;

		Isolate* m_pIsolate;

		Locker* m_pLocker;

    }; // end class AudioEngine
    
} // end namespace Audio

