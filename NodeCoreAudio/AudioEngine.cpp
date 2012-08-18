///////////////////////////////////////////////////////////
//
// AudioEngine.cpp: Core audio functionality
// 
//////////////////////////////////////////////////////////////////////////////
// DevStudio!namespace Audio DevStudio!class AudioEngine
#include "AudioEngine.h"
#include "portaudio.h"
#include <v8.h>
#include <node_internals.h>
#include <node_object_wrap.h>
using namespace v8;

Persistent<Function> Audio::AudioEngine::constructor;

namespace {
	const int SLEEP_SAFETY_FACTOR_MS = 4,
			  SLEEP_TIME = 1000 * (FRAMES_PER_BUFFER / SAMPLE_RATE) + SLEEP_SAFETY_FACTOR_MS;

	void nativeSleep( unsigned int msecs ) {
		#if defined( _WIN32  )
				Sleep( msecs );
		#elif defined( __unix__ )
				usleep( msecs * 1000 );
		#else
				usleep( msecs * 1000 );
		#endif
	}
} // end namespace

//////////////////////////////////////////////////////////////////////////////
/*! Initalize */
Audio::AudioEngine::AudioEngine( Local<Function>& audioCallback ) :
	m_audioCallback(Persistent<Function>::New(audioCallback)),
	m_uNumInputChannels(2),
	m_uNumOutputChannels(2),
	m_bNewAudioData(false),
	m_uSleepTime(SLEEP_TIME) {

	// Initialize our audio core
	PaError initErr = Pa_Initialize();
	if( initErr != paNoError ) 
		ThrowException( Exception::TypeError(String::New("Failed to initialize audio engine")) );

	PaError openStreamErr;

	PaStreamParameters inputParameters, outputParameters;

	// Setup default input device
	inputParameters.device = Pa_GetDefaultInputDevice();
	if (inputParameters.device == paNoDevice) {
		ThrowException( Exception::TypeError(String::New("Error: No default input device")) );
	}

	// Stereo input
	inputParameters.channelCount = 2;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	// Allocate our input buffers
	m_fInputProcessSamples.resize( inputParameters.channelCount );
	m_fInputNonProcessSamples.resize( inputParameters.channelCount );	
	for( int iChannel=0; iChannel<m_fInputNonProcessSamples.size(); ++iChannel ) {
		m_fInputProcessSamples[iChannel].resize( FRAMES_PER_BUFFER );
		m_fInputNonProcessSamples[iChannel].resize( FRAMES_PER_BUFFER );
	} // end for each channel

	// Setup default output device
	outputParameters.device = Pa_GetDefaultOutputDevice();
	if (outputParameters.device == paNoDevice) {
		ThrowException( Exception::TypeError(String::New("Error: No default output device")) );
	}
	
	// Stereo output
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Allocate our output buffers
	m_fOutputProcessSamples.resize( outputParameters.channelCount );
	m_fOutputNonProcessSamples.resize( outputParameters.channelCount );	
	for( int iChannel=0; iChannel<m_fInputNonProcessSamples.size(); ++iChannel ) {
		m_fOutputProcessSamples[iChannel].resize( FRAMES_PER_BUFFER );
		m_fOutputNonProcessSamples[iChannel].resize( FRAMES_PER_BUFFER );
	} // end for each channel

	// Open an audio I/O stream. 
	openStreamErr = Pa_OpenStream(  &m_pStream,
									&inputParameters,
									&outputParameters,
									SAMPLE_RATE,
									FRAMES_PER_BUFFER,
									paClipOff,				// We won't output out of range samples so don't bother clipping them
									audioCallbackSource,	// this is your callback function
									this );					// This is a pointer that will be passed to your callback*/

	if( openStreamErr != paNoError ) 
		ThrowException( Exception::TypeError(String::New("Failed to open audio stream")) );

	// Start the audio stream
	PaError startStreamErr = Pa_StartStream( m_pStream );

	if( startStreamErr != paNoError ) 
		ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

	//Pa_Sleep(2*1000);

	//Pa_Terminate();
} // end AudioEngine::AudioEngine()


//////////////////////////////////////////////////////////////////////////////
/*! Our main audio callback */
int Audio::AudioEngine::audioCallback( const void *input, void *output, unsigned long uSampleFrames, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags ) {

	float* outputSamples= reinterpret_cast<float *>( output ),
		 * inputSamples= reinterpret_cast<float *>( output );

	// If the number of samples we get here isn't right, just copy input to output and get out
	if( uSampleFrames != FRAMES_PER_BUFFER ) {
		for( int iChannel=0; iChannel<m_fInputNonProcessSamples.size(); ++iChannel )
			copyBuffer( uSampleFrames, inputSamples, outputSamples );
		return 0;
	}
	
	// Copy our incoming samples into our non-process buffers
	for( int iChannel=0; iChannel<m_fInputNonProcessSamples.size(); ++iChannel )
		copyBuffer( uSampleFrames, inputSamples, &m_fInputNonProcessSamples[iChannel][0] );

	// Go to sleep, and hope our non-processing thread delivers data!
	Pa_Sleep( m_uSleepTime );

	// Copy the non-processing audio buffer back to the output samples
	copyBuffer( uSampleFrames, &m_fOutputNonProcessSamples[0][0], outputSamples );
	
	return 0;
} // end AudioEngine::audioCallback()


//////////////////////////////////////////////////////////////////////////////
/*! Our node.js instantiation function */
void Audio::AudioEngine::Init(v8::Handle<v8::Object> target) {
	// Prepare constructor template
	Local<FunctionTemplate> functionTemplate = FunctionTemplate::New (Audio::AudioEngine::New );
	functionTemplate->SetClassName( String::NewSymbol("AudioEngine") );
	functionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );

	// Prototype
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("isActive"), FunctionTemplate::New(Audio::AudioEngine::IsActive)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("processIfNewData"), FunctionTemplate::New(Audio::AudioEngine::ProcessIfNewData)->GetFunction() );

	constructor = Persistent<Function>::New( functionTemplate->GetFunction() );
} // end AudioEngine::Init()


//////////////////////////////////////////////////////////////////////////////
/*! Instantiate from instance factory */
v8::Handle<v8::Value> Audio::AudioEngine::NewInstance( const v8::Arguments& args ) {
	HandleScope scope;

	const unsigned argc = 1;
	Handle<Value> argv[argc] = { args[0] };
	Local<Object> instance = constructor->NewInstance( argc, argv );

	return scope.Close(instance);
} // end AudioEngine::NewInstance()


//////////////////////////////////////////////////////////////////////////////
/*! Our V8 new operator */
v8::Handle<v8::Value> Audio::AudioEngine::New( const v8::Arguments& args ) {
	HandleScope scope;

	if (!args[0]->IsFunction()) {
		return ThrowException( Exception::TypeError(String::New("Callback function required")) );
	}

	Local<Function> callback = Local<Function>::Cast( args[0] );
	
	AudioEngine* engine = new AudioEngine( callback );
	engine->Wrap( args.This() );

	return scope.Close( args.This() );
} // end AudioEngine::New()


//////////////////////////////////////////////////////////////////////////////
/*! Returns whether the audio stream active */
v8::Handle<v8::Value> Audio::AudioEngine::IsActive( const v8::Arguments& args ) {
	HandleScope scope;

	AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );
	
	if( Pa_IsStreamActive(engine->m_pStream) )	
		return scope.Close( Boolean::New(true) );
	else
		return scope.Close( Boolean::New(false) );
} // end AudioEngine::IsActive()


//////////////////////////////////////////////////////////////////////////////
/*! Hands audio to a javascript callback if we have new data */
v8::Handle<v8::Value> Audio::AudioEngine::ProcessIfNewData( const v8::Arguments& args ) {
	HandleScope scope;

	if (!args[0]->IsFunction()) {
		return ThrowException( Exception::TypeError(String::New("Callback function required")) );
	}

	Local<Function> callback = Local<Function>::Cast( args[0] );

	AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	// Just return if we don't have any new audio data
// 	if( !engine->m_bNewAudioData )
// 		return scope.Close( Undefined() );

	engine->m_uSampleFrames= Number::New( FRAMES_PER_BUFFER );

	// Copy input 
	engine->m_inputBuffer = v8::Array::New( FRAMES_PER_BUFFER );
	for (int iSample = 0; iSample < (signed)FRAMES_PER_BUFFER; iSample++) {
		float fSample= engine->m_fInputNonProcessSamples[0][iSample];
		engine->m_inputBuffer->Set( v8::Number::New(iSample), v8::Number::New(fSample) );
	}

	// Copy output 
	engine->m_outputBuffer = v8::Array::New( FRAMES_PER_BUFFER );
	for (int iSample = 0; iSample < (signed)FRAMES_PER_BUFFER; iSample++) {
		float fSample= engine->m_fOutputNonProcessSamples[0][iSample];
		engine->m_outputBuffer->Set(v8::Number::New(iSample), v8::Number::New(fSample));
	}

	//engine->wrapObject( v8::Array::New( FRAMES_PER_BUFFER ) );

	vector<float> pfloats;

	const unsigned argc = 2;
	Local<Value> argv[argc] = { Local<Value>::New(engine->m_uSampleFrames), engine->m_inputBuffer };

	Handle<Function> func = Handle<Function>::Cast( args[0] );
	Handle<Value> result = func->Call( args.This(), argc, argv );

	//engine->m_audioCallback->Call( Context::GetCurrent()->Global(), argc, argv );

	return scope.Close( Undefined() );
} // end AudioEngine::ProcessIfNewData()


//////////////////////////////////////////////////////////////////////////////
/*! Wraps a handle into an object */
void Audio::AudioEngine::wrapObject( v8::Handle<v8::Object> object ) {
	ObjectWrap::Wrap( object );
} // end AudioEngine::wrapObject()


//////////////////////////////////////////////////////////////////////////////
/*! Copy one buffer into another */
void Audio::AudioEngine::copyBuffer( int uSampleFrames, float* sourceBuffer, float* destBuffer, bool bZeroSource ) {
	
	// TODO: Vectorize this!
	for( int iSample=0; iSample<uSampleFrames; ++iSample ) {
		destBuffer[iSample] = sourceBuffer[iSample];
	} // end for each sample

	if( bZeroSource ) {
		for( int iSample=0; iSample<uSampleFrames; ++iSample ) {
			sourceBuffer[iSample] = 0.0f;
		} // end for each sample
	} // end if zeroing source buffer

} // end AudioEngine::copyBuffer()


//////////////////////////////////////////////////////////////////////////////
/*! Destructor */
Audio::AudioEngine::~AudioEngine() {
	PaError err = Pa_Terminate();
	if( err != paNoError )
		printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );

	ThrowException( Exception::TypeError(String::New("Being destructed")) );
} // end AudioEngine::~AudioEngine()
