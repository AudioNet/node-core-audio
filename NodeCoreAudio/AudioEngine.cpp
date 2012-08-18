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
	Pa_Initialize();

	PaStream *stream;
	PaError err;

	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream( &stream,
								0,          /* no input channels */
								2,          /* stereo output */
								paFloat32,  /* 32 bit floating point output */
								SAMPLE_RATE,
								256,        /* frames per buffer */
								audioCallbackSource, /* this is your callback function */
								this ); /*This is a pointer that will be passed to your callback*/

} // end AudioEngine::AudioEngine()


//////////////////////////////////////////////////////////////////////////////
/*! Our main audio callback */
int Audio::AudioEngine::audioCallback( const void *input, void *output, unsigned long uSampleFrames, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags ) {
	std::vector<float> teste;

	m_uSampleFrames= Number::New( uSampleFrames );
	//m_inputBuffer= Number::New( teste );
	//m_outputBuffer= Array::New( outputBuffer );
	float* outputSamples= reinterpret_cast<float *>( output );

	// Copy output 
	m_outputBuffer = v8::Array::New( uSampleFrames );
	for (int iSample = 0; iSample < (signed)uSampleFrames; iSample++) {
		float fSample= outputSamples[iSample];
		m_outputBuffer->Set(v8::Number::New(iSample), v8::Number::New(fSample));
	}

	//context->Global()->Set(v8::String::New("arguments"), outputBuffer);

	vector<float> pfloats;

	ObjectWrap::Wrap( m_outputBuffer );

	const unsigned argc = 3;
	Local<Value> argv[argc] = { Local<Value>::New(m_uSampleFrames), m_inputBuffer, m_outputBuffer };
	//cb->Call(Context::GetCurrent()->Global(), argc, argv);

	m_audioCallback->Call( Context::GetCurrent()->Global(), argc, argv );

	return 0;
} // end AudioEngine::audioCallback()


//////////////////////////////////////////////////////////////////////////////
/*! Our node.js instantiation function */
void Audio::AudioEngine::Init(v8::Handle<v8::Object> target) {
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("AudioEngine"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
// 	tpl->PrototypeTemplate()->Set(String::NewSymbol("plusOne"),
// 		FunctionTemplate::New(PlusOne)->GetFunction());

	constructor = Persistent<Function>::New(tpl->GetFunction());
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
	
	Local<Function> callback = Local<Function>::Cast(args[0]);
	
	AudioEngine* engine = new AudioEngine( callback );

	return args.This();
} // end AudioEngine::New()


//////////////////////////////////////////////////////////////////////////////
/*! Destructor */
Audio::AudioEngine::~AudioEngine() {
	
} // end AudioEngine::~AudioEngine()
