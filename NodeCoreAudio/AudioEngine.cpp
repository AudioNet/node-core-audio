//////////////////////////////////////////////////////////////////////////////
//
// AudioEngine.cpp: Core audio functionality
// 
//////////////////////////////////////////////////////////////////////////////
// DevStudio!namespace Audio DevStudio!class AudioEngine


#include "AudioEngine.h"
#include "portaudio.h"
#include <v8.h>
#include <uv.h>
#include <node_internals.h>
#include <node_object_wrap.h>
#include <stdlib.h>
using namespace v8;

Persistent<Function> Audio::AudioEngine::constructor;

//////////////////////////////////////////////////////////////////////////////
/*! Initialize */
Audio::AudioEngine::AudioEngine( Local<Function>& callback, Local<Object> options, bool withThread ) :
    audioJsCallback(Persistent<Function>::New(callback)),
	m_uSampleSize(4),
	m_pIsolate(Isolate::GetCurrent()),
	m_pLocker(new Locker(Isolate::GetCurrent())) {

    PaError openStreamErr;
    m_pPaStream = NULL;

    //set defaults
    m_uInputChannels = 2;
    m_uOutputChannels = 2;
    m_uSamplesPerBuffer = DEFAULT_FRAMES_PER_BUFFER;
    m_uNumCachedOutputSamples = 0;
    m_uSampleFormat = 1;
    m_bInterleaved = false;

    m_uSampleRate = DEFAULT_SAMPLE_RATE;

    m_cachedInputSampleBlock = NULL;
    m_cachedOutputSampleBlock = NULL;

    m_bInputOverflowed = 0;
    m_bOutputUnderflowed = 0;

	// Create V8 objects to hold our buffers
	m_hInputBuffer = Array::New( m_uInputChannels );
	for( int iChannel=0; iChannel<m_uInputChannels; iChannel++ )
		m_hInputBuffer->Set( iChannel, Array::New(m_uSamplesPerBuffer) );

    // Initialize our audio core
    PaError initErr = Pa_Initialize();

    m_uInputDevice = Pa_GetDefaultInputDevice();
    if (m_uInputDevice == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default input device")) );
    };

    m_uOutputDevice = Pa_GetDefaultOutputDevice();
    if (m_uOutputDevice == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default output device")) );
    }

    if( initErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to initialize audio engine")) );

    applyOptions(options);
    
    fprintf( stderr, "input :%d\n", m_uInputDevice );
    fprintf( stderr, "output :%d\n", m_uOutputDevice );
    fprintf( stderr, "rate :%d\n", m_uSampleRate );
    fprintf( stderr, "format :%d\n", m_uSampleFormat );
    fprintf( stderr, "size :%ld\n", sizeof(float) );
    fprintf( stderr, "inputChannels :%d\n", m_uInputChannels );
    fprintf( stderr, "outputChannels :%d\n", m_uOutputChannels );
    fprintf( stderr, "interleaved :%d\n", m_bInterleaved );    

    // Open an audio I/O stream. 
    openStreamErr = Pa_OpenStream(  &m_pPaStream,
                                    &m_inputParams,
                                    &m_outputParams,
                                    m_uSampleRate,
                                    m_uSamplesPerBuffer,
                                    paClipOff,
                                    NULL,
                                    NULL );

    if( openStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream")) );

    // Start the audio stream
    PaError startStreamErr = Pa_StartStream( m_pPaStream );

    if( startStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

    uv_thread_create( &ptStreamThread, (void (__cdecl *)(void *))Audio::AudioEngine::runAudioLoop, (void*)this );

} // end Constructor


//////////////////////////////////////////////////////////////////////////////
/*! Gets options */
v8::Handle<v8::Value> Audio::AudioEngine::GetOptions(const v8::Arguments& args){
    Locker v8Locker;
	HandleScope scope;

    Local<Object> options = Object::New();

    AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );
    
    options->Set(String::New("inputChannels"), Number::New(pEngine->m_uInputChannels));
    options->Set(String::New("outputChannels"), Number::New(pEngine->m_uOutputChannels));

    options->Set(String::New("inputDevice"), Number::New(pEngine->m_uInputDevice));
    options->Set(String::New("outputDevice"), Number::New(pEngine->m_uOutputDevice));

    options->Set(String::New("sampleRate"), Number::New(pEngine->m_uSampleRate));
    options->Set(String::New("sampleFormat"), Number::New(pEngine->m_uSampleFormat));

    options->Set(String::New("framesPerBuffer"), Number::New(pEngine->m_uSamplesPerBuffer));
    options->Set(String::New("interleaved"), Boolean::New(pEngine->m_bInterleaved));
    
    return scope.Close(options);
} // end GetOptions


//////////////////////////////////////////////////////////////////////////////
/*! Set options, restarts audio stream */
v8::Handle<v8::Value> Audio::AudioEngine::SetOptions( const v8::Arguments& args ) {
	HandleScope scope;

	Local<Object> options;

	if( args.Length() > 0 ) {
		if( !args[0]->IsObject() ) {
			return scope.Close( ThrowException(Exception::TypeError(String::New("First argument should be an object."))) );
		}

		options = Local<Object>::Cast( args[0] );
	} else {
		return scope.Close( Exception::TypeError(String::New("First argument does not exist.")) );
	}

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );
	pEngine->applyOptions( options );

	return scope.Close( Undefined() );
} // end AudioEngine::SetOptions()x


//////////////////////////////////////////////////////////////////////////////
/*! Sets the given options and restarts the audio stream if necessary */
void Audio::AudioEngine::applyOptions( Local<Object> options ) {

	if( options->HasOwnProperty(String::New("inputDevice")) )
		m_uInputDevice = options->Get(String::New("inputDevice"))->ToInteger()->Value();
	if( options->HasOwnProperty(String::New("outputDevice")) )
		m_uOutputDevice = options->Get(String::New("outputDevice"))->ToInteger()->Value();
	if( options->HasOwnProperty(String::New("inputChannels")) )
		m_uInputChannels = options->Get(String::New("inputChannels"))->ToInteger()->Value();
	if( options->HasOwnProperty(String::New("outputChannels")) )
		m_uOutputChannels = options->Get(String::New("outputChannels"))->ToInteger()->Value();
	if( options->HasOwnProperty(String::New("framesPerBuffer")) )
		m_uSamplesPerBuffer = options->Get(String::New("framesPerBuffer"))->ToInteger()->Value();
	if( options->HasOwnProperty(String::New("interleaved")) )
		m_bInterleaved = options->Get(String::New("interleaved"))->ToBoolean()->Value();

    if( options->HasOwnProperty(String::New("sampleFormat")) ) {
        switch( options->Get(String::New("sampleFormat"))->ToInteger()->Value() ){
            case 1: m_uSampleFormat = paFloat32; m_uSampleSize = 4; break;
            case 2: m_uSampleFormat = paInt32; m_uSampleSize = 4; break;
            case 4: m_uSampleFormat = paInt24; m_uSampleSize = 3; break;
            case 8: m_uSampleFormat = paInt16; m_uSampleSize = 2; break;
            case 10: m_uSampleFormat = paInt8; m_uSampleSize = 1; break;
            case 20: m_uSampleFormat = paUInt8; m_uSampleSize = 1; break;
        }
    }

    // Setup our input and output parameters
    m_inputParams.device = m_uInputDevice;
    m_inputParams.channelCount = m_uInputChannels;
    m_inputParams.sampleFormat = m_uSampleFormat;
    m_inputParams.suggestedLatency = Pa_GetDeviceInfo( m_inputParams.device )->defaultLowInputLatency;
    m_inputParams.hostApiSpecificStreamInfo = NULL;

	m_outputParams.device = m_uOutputDevice;
	m_outputParams.channelCount = m_uOutputChannels;
	m_outputParams.sampleFormat = m_uSampleFormat;
	m_outputParams.suggestedLatency = Pa_GetDeviceInfo( m_outputParams.device )->defaultLowOutputLatency;
	m_outputParams.hostApiSpecificStreamInfo = NULL;

	// Clear out our temp buffer blocks
    if( m_cachedInputSampleBlock != NULL )
        free( m_cachedInputSampleBlock );
	if( m_cachedOutputSampleBlock != NULL )
		free( m_cachedOutputSampleBlock );

	// Allocate some new space for our temp buffer blocks
    m_cachedInputSampleBlock = (char*)malloc( m_uSamplesPerBuffer * m_uInputChannels * m_uSampleSize );
	m_cachedOutputSampleBlock = (char*)malloc( m_uSamplesPerBuffer * m_uOutputChannels * m_uSampleSize );

    if( m_pPaStream != NULL && Pa_IsStreamActive(m_pPaStream) )
        restartStream();

} // end AudioEngine::applyOptions()


/**
 * Prepares the input/output buffers and calls the javascript callback.
 */
void Audio::AudioEngine::callCallback(uv_work_t* request, int status) {

	AudioEngine *engine = (AudioEngine*)request->data;
	
	engine->GetIsolate()->Enter();
	
    engine->getInputBuffer();
	
    Local<Value> argv[3] = {
        Local<Value>::New(engine->m_hInputBuffer),
        Local<Value>::New(Boolean::New(engine->m_bInputOverflowed)),
        Local<Value>::New(Boolean::New(engine->m_bOutputUnderflowed))
    };
	
    Handle<Array> result = Handle<Array>::Cast(engine->audioJsCallback->Call(engine->audioJsCallback, 3, argv));

    if (result->IsArray()){
        engine->queueOutputBuffer(result);
    }
	
 	engine->GetIsolate()->Exit();

    uv_mutex_unlock(&engine->m_mutex);
	
} // end callCallback


/**
 * Returns an v8 Array based on the cachedInput.
 */
Handle<Array> Audio::AudioEngine::getInputBuffer(){
    int iSample = 0, iChannel = 0;

	Locker v8Locker;
	HandleScope scope;

    if (m_bInterleaved){
        m_hInputBuffer = Array::New( m_uInputChannels * m_uSamplesPerBuffer );

        while(iSample < m_uSamplesPerBuffer*m_uInputChannels){

            m_hInputBuffer->Set(
                iSample,
                getSample(iSample)
            );

            iSample++;
        }

    } else {
		m_hInputBuffer = Array::New( m_uInputChannels );
		for( iChannel=0; iChannel<m_uInputChannels; iChannel++ ) {
			auto tempBuffer = Local<Array>( Array::New(m_uSamplesPerBuffer) );

			for( iSample=0; iSample<m_uSamplesPerBuffer; iSample++ ) {
				tempBuffer->Set( iSample, getSample(iSample) );
			}

			m_hInputBuffer->Set( iChannel, tempBuffer );
		}
    }

    return scope.Close( m_hInputBuffer );
} // end InputToArray

/**
 * Returns the samples recorded from the soundcard as v8 Number.
 */
Handle<Number> Audio::AudioEngine::getSample( int position ){
    
	Locker v8Locker;
	HandleScope scope;

    Handle<Number> sample;

    switch (m_uSampleFormat){
        case paFloat32: {
			float fValue = ((float*)m_cachedInputSampleBlock)[position];

            sample = Number::New( fValue ); 
			break; 
		} case paInt32:
            sample = Integer::New(((int*)m_cachedInputSampleBlock)[position]); break;
        case paInt24:
            sample = Integer::New(
                (m_cachedInputSampleBlock[3*position + 0] << 16)
              + (m_cachedInputSampleBlock[3*position + 1] << 8)
              + (m_cachedInputSampleBlock[3*position + 2])
            );
            break;
        case paInt16:
            sample = Integer::New(((int16_t*)m_cachedInputSampleBlock)[position]); break;
        default:
            sample = Integer::New(m_cachedInputSampleBlock[position]*-1); break;
    }

    return scope.Close( sample );
} // end getSample


/**
 * Sets a v8 sample value to a position for output.
 */
void Audio::AudioEngine::setSample( int position, Handle<Value> sample ) {

    int temp;
    switch (m_uSampleFormat){
        case paFloat32:
            ((float *)m_cachedOutputSampleBlock)[position] = (float)sample->NumberValue();
            break;
        case paInt32:
            ((int *)m_cachedOutputSampleBlock)[position] = (int)sample->NumberValue();
            break;
        case paInt24:

            temp = (int)sample->NumberValue();
            m_cachedOutputSampleBlock[3*position + 0] = temp >> 16;
            m_cachedOutputSampleBlock[3*position + 1] = temp >> 8;
            m_cachedOutputSampleBlock[3*position + 2] = temp;

            break;
        case paInt16:
            ((int16_t *)m_cachedOutputSampleBlock)[position] = (int16_t)sample->NumberValue();
            break;
        default:
            m_cachedOutputSampleBlock[position] = (char)sample->NumberValue();
            break;
    }
} // end setSample


/**
 * Queues the buffer for being used in the streamThread.
 */
void Audio::AudioEngine::queueOutputBuffer( Handle<Array> result ) {
	// Reset our record of the number of cached output samples
    m_uNumCachedOutputSamples = 0;

    if( m_bInterleaved ) {
		for( int iSample=0; iSample<m_uSamplesPerBuffer*m_uOutputChannels; ++iSample )
			setSample( iSample, result->Get(iSample) );

        m_uNumCachedOutputSamples = result->Length()/m_uOutputChannels;

    } else {
		// Validate the structure of the output buffer array
		if( !result->Get(0)->IsArray() ) {
			ThrowException( Exception::TypeError(String::New("Output buffer not properly setup, 0th channel is not an array")) );
			return;
		}
		
        Handle<Array> item;

		for( int iChannel=0; iChannel<m_uOutputChannels; ++iChannel ) {
			for( int iSample=0; iSample<m_uSamplesPerBuffer; ++iSample ) {
				
				item = Handle<Array>::Cast( result->Get(iChannel) );

				if( item->IsArray() ) {
					if( item->Length() > m_uNumCachedOutputSamples )
						m_uNumCachedOutputSamples = item->Length();

					setSample( iSample, item->Get(iSample) );
				}
			}
		}
    }

} // end queueOutputBuffer


//////////////////////////////////////////////////////////////////////////////
/*! Run the main blocking audio loop */
void* Audio::AudioEngine::runAudioLoop( void* data ){

    AudioEngine* pEngine = (AudioEngine*)data;
    PaError error;

	assert( pEngine->m_pPaStream );

    while( true ) {

        error = Pa_ReadStream( pEngine->m_pPaStream, pEngine->m_cachedInputSampleBlock, pEngine->m_uSamplesPerBuffer );

        pEngine->m_bInputOverflowed = (error != paNoError);

		uv_work_t* request = new uv_work_t;
		request->data = pEngine;

		uv_mutex_trylock( &pEngine->m_mutex );

		int statusInt = 0;
		callCallback( request, statusInt );
        uv_mutex_lock( &pEngine->m_mutex ); //wait, until js call is done

        if( pEngine->m_uNumCachedOutputSamples > 0 ) {
            error = Pa_WriteStream( pEngine->m_pPaStream, pEngine->m_cachedOutputSampleBlock, pEngine->m_uNumCachedOutputSamples );
            pEngine->m_bOutputUnderflowed = (error != paNoError);
            pEngine->m_uNumCachedOutputSamples = 0;
        }
    }
} // end streamThread


//////////////////////////////////////////////////////////////////////////////
/*! Initialize our node object */
void Audio::AudioEngine::Init( v8::Handle<v8::Object> target ) {
	// Prepare constructor template
	Local<FunctionTemplate> functionTemplate = FunctionTemplate::New (Audio::AudioEngine::New );
	functionTemplate->SetClassName( String::NewSymbol("AudioEngine") );
	functionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );

	// Get
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("isActive"), FunctionTemplate::New(Audio::AudioEngine::IsActive)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getDeviceName"), FunctionTemplate::New(Audio::AudioEngine::GetDeviceName)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getNumDevices"), FunctionTemplate::New(Audio::AudioEngine::GetNumDevices)->GetFunction() );

	// Set
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("setOptions"), FunctionTemplate::New(Audio::AudioEngine::SetOptions)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getOptions"), FunctionTemplate::New(Audio::AudioEngine::GetOptions)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("write"), FunctionTemplate::New(Audio::AudioEngine::Write)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("read"), FunctionTemplate::New(Audio::AudioEngine::Read)->GetFunction() );

	constructor = Persistent<Function>::New( functionTemplate->GetFunction() );
} // end AudioEngine::Init()


//////////////////////////////////////////////////////////////////////////////
/*! Create a new instance of the audio engine */
v8::Handle<v8::Value> Audio::AudioEngine::NewInstance(const v8::Arguments& args) {
	HandleScope scope;

	unsigned argc = args.Length();

	if( argc > 2 ) 
		argc = 2;

	Handle<Value>* argv = new Handle<Value>[argc];

	argv[0] = args[0];
	if( argc > 1 )
		argv[1] = args[1];

	Local<Object> instance = constructor->NewInstance( argc, argv );

	return scope.Close( instance );
} // end AudioEngine::NewInstance()


/**
 * The V8 new operator.
 */
v8::Handle<v8::Value> Audio::AudioEngine::New( const v8::Arguments& args ) {
    HandleScope scope;

    Local<Function> callback;
    Local<Object> options;
    bool bWithThread = false;

    if( args.Length() > 0 ) {
        if( !args[0]->IsObject() )
            return scope.Close( ThrowException(Exception::TypeError(String::New("First argument must be an object."))) );
        else
            options = Local<Object>::Cast( args[0] );
    } else {
        options = Object::New();
    }
    
    if( args.Length() > 1 ) {
        if( args[1]->IsFunction() ) {
            callback = Local<Function>::Cast( args[1] );
            bWithThread = true;
        }
    }

    AudioEngine* pEngine = new AudioEngine( callback, options, bWithThread );
    pEngine->Wrap( args.This() );

    return scope.Close( args.This() );
} // end New()


//////////////////////////////////////////////////////////////////////////////
/*! Write samples to the current audio device */
v8::Handle<v8::Value> Audio::AudioEngine::Write( const v8::Arguments& args ) {
	HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	if (args.Length() > 1 || !args[0]->IsArray()){
		return scope.Close( ThrowException(Exception::TypeError(String::New("First argument should be an array."))) );
	}

	pEngine->queueOutputBuffer( Local<Array>::Cast(args[0]) );

	Handle<Boolean> result = Boolean::New( false );

	if( pEngine->m_uNumCachedOutputSamples > 0 ) {
		if( !Pa_WriteStream(pEngine->m_pPaStream, pEngine->m_cachedOutputSampleBlock, pEngine->m_uNumCachedOutputSamples) )
			result = Boolean::New( true );
		pEngine->m_uNumCachedOutputSamples = 0;
	}

	return scope.Close( result );
} // end AudioEngine::Write()


//////////////////////////////////////////////////////////////////////////////
/*! Read samples from the current audio device */
v8::Handle<v8::Value> Audio::AudioEngine::Read( const v8::Arguments& args ) {
	HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	Pa_ReadStream( pEngine->m_pPaStream, pEngine->m_cachedInputSampleBlock, pEngine->m_uSamplesPerBuffer );

	Handle<Array> input = pEngine->getInputBuffer();

	return scope.Close( input );
} // end AudioEngine::Read()


//////////////////////////////////////////////////////////////////////////////
/*! Returns whether the PortAudio stream is active */
v8::Handle<v8::Value> Audio::AudioEngine::IsActive( const v8::Arguments& args ) {
	HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	if( Pa_IsStreamActive(pEngine->m_pPaStream) )
		return scope.Close( Boolean::New(true) );
	else
		return scope.Close( Boolean::New(false) );
} // end AudioEngine::IsActive()


//////////////////////////////////////////////////////////////////////////////
/*! Get the name of an audio device with a given ID number */
v8::Handle<v8::Value> Audio::AudioEngine::GetDeviceName( const v8::Arguments& args ) {
	HandleScope scope;

	if( !args[0]->IsNumber() ) {
		return scope.Close( ThrowException(Exception::TypeError(String::New("getDeviceName() requires a device index"))) );
	}

	Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

	const PaDeviceInfo* pDeviceInfo = Pa_GetDeviceInfo( (PaDeviceIndex)deviceIndex->NumberValue() );

	return scope.Close( String::New(pDeviceInfo->name) );
} // end AudioEngine::GetDeviceName()


//////////////////////////////////////////////////////////////////////////////
/*! Get the number of available devices */
v8::Handle<v8::Value> Audio::AudioEngine::GetNumDevices( const v8::Arguments& args ) {
	HandleScope scope;

    int deviceCount = Pa_GetDeviceCount();

    return scope.Close( Number::New(deviceCount) );
} // end AudioEngine::GetNumDevices()


//////////////////////////////////////////////////////////////////////////////
/*! Closes and reopens the PortAudio stream */
void Audio::AudioEngine::restartStream() {
	PaError error;

	// Stop the audio stream
	error = Pa_StopStream( m_pPaStream );
	
	if( error != paNoError )
		ThrowException( Exception::TypeError(String::New("Failed to stop audio stream")) );

	// Close the audio stream
    error = Pa_CloseStream( m_pPaStream );

	if( error != paNoError )
		ThrowException( Exception::TypeError(String::New("Failed to close audio stream")) );

	// Open an audio stream. 
	error = Pa_OpenStream(  &m_pPaStream,
        &m_inputParams,
        &m_outputParams,
        m_uSampleRate,
        m_uSamplesPerBuffer,
        paClipOff,              // We won't output out of range samples so don't bother clipping them
        NULL,
        NULL );

    if( error != paNoError ) {
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream :(")) );
    }

	// Start the audio stream
    error = Pa_StartStream( m_pPaStream );

    if( error != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

} // end AudioEngine::restartStream()


//////////////////////////////////////////////////////////////////////////////
/*! Wraps a handle into an object */
void Audio::AudioEngine::wrapObject( v8::Handle<v8::Object> object ) {
	ObjectWrap::Wrap( object );
} // end AudioEngine::wrapObject()


//////////////////////////////////////////////////////////////////////////////
/*! OOL Destructor */
Audio::AudioEngine::~AudioEngine() {
	// Kill PortAudio
	PaError err = Pa_Terminate();

	if( err != paNoError )
		fprintf( stderr, "PortAudio error: %s\n", Pa_GetErrorText( err ) );
} // end AudioEngine::~AudioEngine()