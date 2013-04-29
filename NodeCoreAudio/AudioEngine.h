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

        static void Init(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);

		Persistent<Context> GetContext() { return m_v8Context; }
		Isolate* GetIsolate() { return m_pIsolate; }
		Locker* GetLocker() { return m_pLocker; }

        ~AudioEngine();

    private:
        static v8::Persistent<v8::Function> constructor;                
        static v8::Handle<v8::Value> New( const v8::Arguments& args );
        
        static v8::Handle<v8::Value> IsActive( const v8::Arguments& args );
        static v8::Handle<v8::Value> GetDeviceName( const v8::Arguments& args );
        static v8::Handle<v8::Value> GetNumDevices( const v8::Arguments& args );

        static v8::Handle<v8::Value> Write( const v8::Arguments& args );
        static v8::Handle<v8::Value> Read( const v8::Arguments& args );

        static v8::Handle<v8::Value> SetOptions( const v8::Arguments& args );
        static v8::Handle<v8::Value> GetOptions( const v8::Arguments& args );

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

        PaStream *paStream;						//!< The PortAudio stream object
        PaStreamParameters inputParameters,		//!< PortAudio stream parameters
                           outputParameters;

		Handle<Array> inputBuffer;				//!< Our pre-allocated input buffer

		uv_thread_t ptStreamThread;				//!< Our stream thread
        uv_thread_t jsAudioThread;				//!< Our JavaScript Audio thread

        /**
         * The mutex that detects whether the v8 javascript callback function is done or not.
         * Since the v8 callback and the streamThread are in different threads, this is needed
         * to be synchronized.
         */
        uv_mutex_t callerThreadSamplesAccess;

        /**
         * Stores the v8 javascript callback function.
         */
        Persistent<Function> audioJsCallback;   //!< We call this with audio data whenever we get it

        /**
         * All options as own var.
         */
        int outputChannels,
            inputChannels,
            inputDevice,
            outputDevice,
            sampleRate,
            framesPerBuffer,
            sampleFormat,
            sampleSize;
        /**
         * Cached output buffer length, so that the streamThread
         * knows whether he should write the buffer to the soundcard or not.
         */
        unsigned int cachedOutputSamplesLength;

        /**
         * Stores whether a overflowed or underflowed stream was there.
         */
        bool inputOverflowed, outputUnderflowed;

        /**
         * Whether or not we handle the output/input as interleaved samples.
         */
        bool interleaved;

        /**
         * The input/output buffer cached for the callCallback method.
         */
        char  *cachedInputSampleBlock,
              *cachedOutputSampleBlock;

		Persistent<Context> m_v8Context;
		Isolate* m_pIsolate;
		Locker* m_pLocker;

    }; // end class AudioEngine
    
} // end namespace Audio

