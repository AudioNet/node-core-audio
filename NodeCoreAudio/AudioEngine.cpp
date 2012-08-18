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

//////////////////////////////////////////////////////////////////////////////
/*! Initalize */
Audio::AudioEngine::AudioEngine( Local<Function>& audioCallback ) :
	m_audioCallback(Persistent<Function>::New(audioCallback)),
	m_uNumInputChannels(2),
	m_uNumOutputChannels(2) {

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
	Isolate::Enter();
	
	m_uSampleFrames= Number::New( uSampleFrames );
	float* outputSamples= reinterpret_cast<float *>( output );

	// Copy output 
	m_outputBuffer = v8::Array::New( uSampleFrames );
	for (int iSample = 0; iSample < (signed)uSampleFrames; iSample++) {
		float fSample= outputSamples[iSample];
		m_outputBuffer->Set(v8::Number::New(iSample), v8::Number::New(fSample));
	}

	vector<float> pfloats;

	ObjectWrap::Wrap( m_outputBuffer );

	const unsigned argc = 3;
	Local<Value> argv[argc] = { Local<Value>::New(m_uSampleFrames), m_inputBuffer, m_outputBuffer };

	m_audioCallback->Call( Context::GetCurrent()->Global(), argc, argv );

	Isolate::Exit;

	return 0;
} // end AudioEngine::audioCallback()


//////////////////////////////////////////////////////////////////////////////
/*! Our node.js instantiation function */
void Audio::AudioEngine::Init(v8::Handle<v8::Object> target) {
	// Prepare constructor template
	Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(Audio::AudioEngine::New);
	functionTemplate->SetClassName(String::NewSymbol("AudioEngine"));
	functionTemplate->InstanceTemplate()->SetInternalFieldCount(1);

	// Prototype
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("plusOne"), FunctionTemplate::New(Audio::AudioEngine::PlusOne)->GetFunction() );
	functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("isActive"), FunctionTemplate::New(Audio::AudioEngine::IsActive)->GetFunction() );

	constructor = Persistent<Function>::New( functionTemplate->GetFunction() );
} // end AudioEngine::Init()


//////////////////////////////////////////////////////////////////////////////
/*! Instantiate from instance factory */
v8::Handle<v8::Value> Audio::AudioEngine::NewInstance(const v8::Arguments& args) {
	HandleScope scope;

	const unsigned argc = 1;
	Handle<Value> argv[argc] = { args[0] };
	Local<Object> instance = constructor->NewInstance(argc, argv);

	return scope.Close(instance);
} // end AudioEngine::NewInstance()


//////////////////////////////////////////////////////////////////////////////
/*! Our V8 new operator */
v8::Handle<v8::Value> Audio::AudioEngine::New(const v8::Arguments& args) {
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
/*! Test */
v8::Handle<v8::Value> Audio::AudioEngine::PlusOne(const v8::Arguments& args) {
	HandleScope scope;

	if (!args[0]->IsFunction()) {
		return ThrowException( Exception::TypeError(String::New("Callback function required")) );
	}

	return scope.Close( Number::New(2) );
} // end AudioEngine::PlusOne()


//////////////////////////////////////////////////////////////////////////////
/*! Returns whether the audio stream active */
v8::Handle<v8::Value> Audio::AudioEngine::IsActive(const v8::Arguments& args) {
	HandleScope scope;

	AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>(args.This());
	
	if( Pa_IsStreamActive(engine->m_pStream) )	
		return scope.Close( Boolean::New(true) );
	else
		return scope.Close( Boolean::New(false) );
} // end AudioEngine::IsActive()


//////////////////////////////////////////////////////////////////////////////
/*! Destructor */
Audio::AudioEngine::~AudioEngine() {
	PaError err = Pa_Terminate();
	if( err != paNoError )
		printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );

	ThrowException( Exception::TypeError(String::New("Being destructed")) );
} // end AudioEngine::~AudioEngine()
