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

#ifdef __linux__
	#include <unistd.h>
	#include <string.h>
#endif

using namespace v8; using namespace std;

Persistent<Function> Audio::AudioEngine::constructor;

static void do_work( void* arg ) {
	Audio::AudioEngine* pEngine = (Audio::AudioEngine*)arg;
	pEngine->RunAudioLoop();
}

//////////////////////////////////////////////////////////////////////////////
/*! Initialize */
Audio::AudioEngine::AudioEngine( Local<Object> options ) :
	m_uSampleSize(4),
	m_pIsolate(Isolate::GetCurrent()),
	m_pLocker(new Locker(Isolate::GetCurrent())),
	m_bOutputIsEmpty(true) {

	PaError openStreamErr;
	m_pPaStream = NULL;

	//set defaults
	m_uInputChannels = 2;
	m_uOutputChannels = 2;
	m_uSamplesPerBuffer = DEFAULT_FRAMES_PER_BUFFER;
	m_uNumCachedOutputSamples = 0;
	m_uSampleFormat = 1;
	m_uNumBuffers = DEFAULT_NUM_BUFFERS;
	m_bInterleaved = false;
	m_bReadMicrophone = true;

	m_uSampleRate = DEFAULT_SAMPLE_RATE;

	m_cachedInputSampleBlock = NULL;
	m_cachedOutputSampleBlock = NULL;
	m_cachedOutputSampleBlockForWriting = NULL;

	m_bInputOverflowed = 0;
	m_bOutputUnderflowed = 0;

	m_uCurrentWriteBuffer = 0;
	m_uCurrentReadBuffer = 0;

	// Create V8 objects to hold our buffers
	m_hInputBuffer = NanNew<Array>( m_uInputChannels );
	for( int iChannel=0; iChannel<m_uInputChannels; iChannel++ )
		m_hInputBuffer->Set( iChannel, NanNew<Array>(m_uSamplesPerBuffer) );

	// Initialize our audio core
	PaError initErr = Pa_Initialize();

	m_uInputDevice = Pa_GetDefaultInputDevice();
	if( m_uInputDevice == paNoDevice ) {
		NanThrowTypeError("Error: No default input device");
	};

	m_uOutputDevice = Pa_GetDefaultOutputDevice();
	if( m_uOutputDevice == paNoDevice ) {
		NanThrowTypeError("Error: No default output device");
	}

	if( initErr != paNoError )
		NanThrowTypeError("Failed to initialize audio engine");

	applyOptions( options );

	fprintf( stderr, "input :%d\n", m_uInputDevice );
	fprintf( stderr, "output :%d\n", m_uOutputDevice );
	fprintf( stderr, "rate :%d\n", m_uSampleRate );
	fprintf( stderr, "format :%d\n", m_uSampleFormat );
	fprintf( stderr, "size :%ld\n", sizeof(float) );
	fprintf( stderr, "inputChannels :%d\n", m_uInputChannels );
	fprintf( stderr, "outputChannels :%d\n", m_uOutputChannels );
	fprintf( stderr, "interleaved :%d\n", m_bInterleaved );
	fprintf( stderr, "uses input: %d\n", m_bReadMicrophone);

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
		NanThrowTypeError("Failed to open audio stream");

	// Start the audio stream
	PaError startStreamErr = Pa_StartStream( m_pPaStream );

	if( startStreamErr != paNoError )
		NanThrowTypeError("Failed to start audio stream");

	uv_mutex_init( &m_mutex );
	uv_thread_create( &ptStreamThread, do_work, (void*)this );

} // end Constructor


//////////////////////////////////////////////////////////////////////////////
/*! Gets options */
//v8::Handle<v8::Value> Audio::AudioEngine::getOptions(const v8::Arguments& args){
NAN_METHOD(Audio::AudioEngine::getOptions){
    NanScope();
	//HandleScope scope;
    NanLocker();

	Local<Object> options = NanNew<Object>();

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	options->Set( NanNew<String>("inputChannels"), NanNew<Number>(pEngine->m_uInputChannels) );
	options->Set( NanNew<String>("outputChannels"), NanNew<Number>(pEngine->m_uOutputChannels) );

	options->Set( NanNew<String>("inputDevice"), NanNew<Number>(pEngine->m_uInputDevice) );
	options->Set( NanNew<String>("outputDevice"), NanNew<Number>(pEngine->m_uOutputDevice) );

	options->Set( NanNew<String>("sampleRate"), NanNew<Number>(pEngine->m_uSampleRate) );
	options->Set( NanNew<String>("sampleFormat"), NanNew<Number>(pEngine->m_uSampleFormat) );
	options->Set( NanNew<String>("framesPerBuffer"), NanNew<Number>(pEngine->m_uSamplesPerBuffer) );
	options->Set( NanNew<String>("numBuffers"), NanNew<Number>(pEngine->m_uNumBuffers) );
	options->Set( NanNew<String>("interleaved"), NanNew<Boolean>(pEngine->m_bInterleaved) );
	options->Set( NanNew<String>("useMicrophone"), NanNew<Boolean>(pEngine->m_bReadMicrophone) );

	NanReturnValue(options);
} // end GetOptions


//////////////////////////////////////////////////////////////////////////////
/*! Set options, restarts audio stream */
//v8::Handle<v8::Value> Audio::AudioEngine::setOptions( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::setOptions){
    NanScope();
	//HandleScope scope;
	Local<Object> options;

	if( args.Length() > 0 ) {
		if( !args[0]->IsObject() ) {
			return NanThrowTypeError("First argument should be an object.");
		}

		options = Local<Object>::Cast( args[0] );
	} else {
        return NanThrowTypeError("First argument does not exist.");
	}

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );
	pEngine->applyOptions( options );

    NanReturnUndefined();
} // end AudioEngine::SetOptions()x


//////////////////////////////////////////////////////////////////////////////
/*! Sets the given options and restarts the audio stream if necessary */
void Audio::AudioEngine::applyOptions( Local<Object> options ) {
	unsigned int oldBufferCount = m_uNumBuffers;
	if( options->HasOwnProperty(NanNew<String>("inputDevice")) )
		m_uInputDevice = (int)options->Get(NanNew<String>("inputDevice"))->ToInteger()->Value();
	if( options->HasOwnProperty(NanNew<String>("outputDevice")) )
		m_uOutputDevice = (int)options->Get(NanNew<String>("outputDevice"))->ToInteger()->Value();
	if( options->HasOwnProperty(NanNew<String>("inputChannels")) )
		m_uInputChannels = (int)options->Get(NanNew<String>("inputChannels"))->ToInteger()->Value();
	if( options->HasOwnProperty(NanNew<String>("outputChannels")) )
		m_uOutputChannels = (int)options->Get(NanNew<String>("outputChannels"))->ToInteger()->Value();
	if( options->HasOwnProperty(NanNew<String>("framesPerBuffer")) )
		m_uSamplesPerBuffer = (int)options->Get(NanNew<String>("framesPerBuffer"))->ToInteger()->Value();
	if ( options->HasOwnProperty(NanNew<String>("numBuffers")) )
		m_uNumBuffers = (int)options->Get(NanNew<String>("numBuffers"))->ToInteger()->Value();

	if( options->HasOwnProperty(NanNew<String>("interleaved")) )
		m_bInterleaved = options->Get(NanNew<String>("interleaved"))->ToBoolean()->Value();
	if ( options->HasOwnProperty(NanNew<String>("useMicrophone")) )
		m_bReadMicrophone = options->Get(NanNew<String>("useMicrophone"))->ToBoolean()->Value();

	if( options->HasOwnProperty(NanNew<String>("sampleFormat")) ) {
		switch( options->Get(NanNew<String>("sampleFormat"))->ToInteger()->Value() ){
			case 0x01: m_uSampleFormat = paFloat32; m_uSampleSize = 4; break;
			case 0x02: m_uSampleFormat = paInt32; m_uSampleSize = 4; break;
			case 0x04: m_uSampleFormat = paInt24; m_uSampleSize = 3; break;
			case 0x08: m_uSampleFormat = paInt16; m_uSampleSize = 2; break;
			case 0x10: m_uSampleFormat = paInt8; m_uSampleSize = 1; break;
			case 0x20: m_uSampleFormat = paUInt8; m_uSampleSize = 1; break;
		}
	}

	// Setup our input and output parameters
	m_inputParams.device = m_uInputDevice;
	m_inputParams.channelCount = m_uInputChannels;
	m_inputParams.sampleFormat = m_uSampleFormat;
	m_inputParams.suggestedLatency = Pa_GetDeviceInfo(m_inputParams.device)->defaultLowInputLatency;
	m_inputParams.hostApiSpecificStreamInfo = NULL;

	m_outputParams.device = m_uOutputDevice;
	m_outputParams.channelCount = m_uOutputChannels;
	m_outputParams.sampleFormat = m_uSampleFormat;
	m_outputParams.suggestedLatency = Pa_GetDeviceInfo(m_outputParams.device)->defaultLowOutputLatency;
	m_outputParams.hostApiSpecificStreamInfo = NULL;

	// Clear out our temp buffer blocks
	if( m_cachedInputSampleBlock != NULL )
		free( m_cachedInputSampleBlock );
	if( m_cachedOutputSampleBlock != NULL ) {
		for (int i = 0; i < oldBufferCount; i++) {
			if ( m_cachedOutputSampleBlock[i] != NULL ) {
				free(m_cachedOutputSampleBlock[i]);
			}
		}
		free( m_cachedOutputSampleBlock );
	}
	if ( m_cachedOutputSampleBlockForWriting != NULL)
		free (m_cachedOutputSampleBlockForWriting);

	// Allocate some new space for our temp buffer blocks
	m_cachedInputSampleBlock = (char*)malloc( m_uSamplesPerBuffer * m_uInputChannels * m_uSampleSize );
	m_cachedOutputSampleBlock = (char**)malloc( sizeof(char*) * m_uNumBuffers);
	for (int i = 0; i < m_uNumBuffers; i++) {
		m_cachedOutputSampleBlock[i] = (char*)malloc( m_uSamplesPerBuffer * m_uOutputChannels * m_uSampleSize );
	}
	m_cachedOutputSampleBlockForWriting = (char*)malloc( m_uSamplesPerBuffer * m_uOutputChannels * m_uSampleSize );
	m_uNumCachedOutputSamples = (unsigned int*)calloc( sizeof(unsigned int), m_uNumBuffers );

	if( m_pPaStream != NULL && Pa_IsStreamActive(m_pPaStream) )
		restartStream();

} // end AudioEngine::applyOptions()

//////////////////////////////////////////////////////////////////////////////
/*! Returns a v8 array filled with input samples */
Handle<Array> Audio::AudioEngine::getInputBuffer() {
//NAN_METHOD(Audio::AudioEngine::getInputBuffer){
    NanEscapableScope();
    //NanScope();
    NanLocker();
	//HandleScope scope;

	if( m_bInterleaved ) {
		m_hInputBuffer = NanNew<Array>( m_uInputChannels * m_uSamplesPerBuffer );

		for( int iSample=0; iSample<m_uSamplesPerBuffer*m_uInputChannels; ++iSample ) {
			m_hInputBuffer->Set( iSample, getSample(iSample) );
		}
	} else {
		m_hInputBuffer = NanNew<Array>( m_uInputChannels );
		for( int iChannel=0; iChannel<m_uInputChannels; iChannel++ ) {
			auto tempBuffer = Local<Array>( NanNew<Array>(m_uSamplesPerBuffer) );

			for( int iSample=0; iSample<m_uSamplesPerBuffer; iSample++ ) {
				tempBuffer->Set( iSample, getSample(iSample) );
			}

			m_hInputBuffer->Set( iChannel, tempBuffer );
		}
	}

	return NanEscapeScope( m_hInputBuffer );
} // end AudioEngine::getInputBuffer()


//////////////////////////////////////////////////////////////////////////////
/*! Returns a sound card sample converted to a v8 Number */
Handle<Number> Audio::AudioEngine::getSample( int position ) {
//NAN_METHOD(Audio::AudioEngine::getSample){
    NanEscapableScope();
    NanLocker();
	//HandleScope scope;

	Handle<Number> sample;

	switch( m_uSampleFormat ) {
	case paFloat32: {
		float fValue = ((float*)m_cachedInputSampleBlock)[position];

		sample = NanNew<Number>( fValue );
		break;
					}
	case paInt32:
		sample = NanNew<Integer>( ((int*)m_cachedInputSampleBlock)[position] );
		break;

	case paInt24:
		sample = NanNew<Integer>(
				(m_cachedInputSampleBlock[3*position + 0] << 16)
			  + (m_cachedInputSampleBlock[3*position + 1] << 8)
			  + (m_cachedInputSampleBlock[3*position + 2]) );
		break;

	case paInt16:
		sample = NanNew<Integer>( ((int16_t*)m_cachedInputSampleBlock)[position] );
		break;

	default:
		sample = NanNew<Integer>( m_cachedInputSampleBlock[position]*-1 );
		break;
	}

    return NanEscapeScope(sample);
} // end AudioEngine::getSample()


//////////////////////////////////////////////////////////////////////////////
/*! Sets a sample in the queued output buffer */
void Audio::AudioEngine::setSample( int position, Handle<Value> sample ) {
	int temp;
	switch( m_uSampleFormat ) {
	case paFloat32:
		((float *)m_cachedOutputSampleBlock[m_uCurrentWriteBuffer])[position] = (float)sample->NumberValue();
		break;

	case paInt32:
		((int *)m_cachedOutputSampleBlock[m_uCurrentWriteBuffer])[position] = (int)sample->NumberValue();
		break;

	case paInt24:
		temp = (int)sample->NumberValue();
		m_cachedOutputSampleBlock[m_uCurrentWriteBuffer][3*position + 0] = temp >> 16;
		m_cachedOutputSampleBlock[m_uCurrentWriteBuffer][3*position + 1] = temp >> 8;
		m_cachedOutputSampleBlock[m_uCurrentWriteBuffer][3*position + 2] = temp;
		break;

	case paInt16:
		((int16_t *)m_cachedOutputSampleBlock[m_uCurrentWriteBuffer])[position] = (int16_t)sample->NumberValue();
		break;

	default:
		m_cachedOutputSampleBlock[m_uCurrentWriteBuffer][position] = (char)sample->NumberValue();
		break;
	}
} // end AudioEngine::setSample()


//////////////////////////////////////////////////////////////////////////////
/*! Queues up an array to be sent to the sound card */
void Audio::AudioEngine::queueOutputBuffer( Handle<Array> result ) {
	// Reset our record of the number of cached output samples
	m_uNumCachedOutputSamples[m_uCurrentWriteBuffer] = 0;

	if( m_bInterleaved ) {
		for( int iSample=0; iSample<m_uSamplesPerBuffer*m_uOutputChannels; ++iSample )
			setSample( iSample, result->Get(iSample) );

		m_uNumCachedOutputSamples[m_uCurrentWriteBuffer] = result->Length()/m_uOutputChannels;

	} else {
		// Validate the structure of the output buffer array
		if( !result->Get(0)->IsArray() ) {
			NanThrowTypeError("Output buffer not properly setup, 0th channel is not an array");
			return;
		}

		Handle<Array> item;

		for( int iChannel=0; iChannel<m_uOutputChannels; ++iChannel ) {
			for( int iSample=0; iSample<m_uSamplesPerBuffer; ++iSample ) {

				item = Handle<Array>::Cast( result->Get(iChannel) );
				if( item->IsArray() ) {
					if( item->Length() > m_uNumCachedOutputSamples[m_uCurrentWriteBuffer] )
						m_uNumCachedOutputSamples[m_uCurrentWriteBuffer] = item->Length();

					setSample( iSample, item->Get(iSample) );
				}
			} // end for each sample
		} // end for each channel
	}
	m_uCurrentWriteBuffer = (m_uCurrentWriteBuffer + 1)%m_uNumBuffers;
} // end AudioEngine::queueOutputBuffer()


//////////////////////////////////////////////////////////////////////////////
/*! Run the main blocking audio loop */
void Audio::AudioEngine::RunAudioLoop(){

	PaError error;

	assert( m_pPaStream );

	while( true ) {
		m_bInputOverflowed = (error != paNoError);

		int numSamples = 0;
		uv_mutex_lock( &m_mutex );
		if (m_uNumCachedOutputSamples[m_uCurrentReadBuffer] > 0) {
			memcpy(m_cachedOutputSampleBlockForWriting, m_cachedOutputSampleBlock[m_uCurrentReadBuffer], m_uNumCachedOutputSamples[m_uCurrentReadBuffer] * m_uOutputChannels * m_uSampleSize);
			numSamples = m_uNumCachedOutputSamples[m_uCurrentReadBuffer];
			m_uNumCachedOutputSamples[m_uCurrentReadBuffer] = 0;
			m_uCurrentReadBuffer = (m_uCurrentReadBuffer + 1)%m_uNumBuffers;
		}
		uv_mutex_unlock( &m_mutex );

		if( numSamples > 0 ) {
			error = Pa_WriteStream( m_pPaStream, m_cachedOutputSampleBlockForWriting, numSamples );
			m_bOutputUnderflowed = (error != paNoError);
		} else {
			m_bOutputUnderflowed = true;
#ifdef __WINDOWS__
			Sleep(1);
#else
			sleep(1);
#endif
		}
	}
} // end AudioEngine::RunAudioLoop()


//////////////////////////////////////////////////////////////////////////////
/*! Initialize our node object */
void Audio::AudioEngine::Init( v8::Handle<v8::Object> target ) {
	// Prepare constructor template
	Local<FunctionTemplate> functionTemplate = NanNew<FunctionTemplate> (Audio::AudioEngine::New );
	functionTemplate->SetClassName( NanNew<String>("AudioEngine") );
	functionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );


    //Local<FunctionTemplate> constructorHandle = NanNew(constructor);
    //target->Set(NanNew<String>("AudioEngine"), functionTemplate->GetFunction());
	
    // Get
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("isActive"), NanNew<FunctionTemplate>(Audio::AudioEngine::isActive)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("getDeviceName"), NanNew<FunctionTemplate>(Audio::AudioEngine::getDeviceName)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("getNumDevices"), NanNew<FunctionTemplate>(Audio::AudioEngine::getNumDevices)->GetFunction() );
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "isActive", Audio::AudioEngine::isActive);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "getDeviceName", Audio::AudioEngine::getDeviceName);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "getNumDevices", Audio::AudioEngine::getNumDevices);

	// Set
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("setOptions"), NanNew<FunctionTemplate>(Audio::AudioEngine::setOptions)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("getOptions"), NanNew<FunctionTemplate>(Audio::AudioEngine::getOptions)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("write"), NanNew<FunctionTemplate>(Audio::AudioEngine::write)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("read"), NanNew<FunctionTemplate>(Audio::AudioEngine::read)->GetFunction() );
	//functionTemplate->PrototypeTemplate()->Set( NanNew<String>("isBufferEmpty"), NanNew<FunctionTemplate>(Audio::AudioEngine::isBufferEmpty)->GetFunction() );
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "setOptions", Audio::AudioEngine::setOptions);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "getOptions", Audio::AudioEngine::getOptions);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "write", Audio::AudioEngine::write);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "read", Audio::AudioEngine::read);
    NODE_SET_PROTOTYPE_METHOD(functionTemplate, "isBufferEmpty", Audio::AudioEngine::isBufferEmpty);

	//constructor = Persistent<Function>::New( functionTemplate->GetFunction() );
    //Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(EOLFinder::New);
    NanAssignPersistent(constructor, functionTemplate->GetFunction());
} // end AudioEngine::Init()


//////////////////////////////////////////////////////////////////////////////
/*! Create a new instance of the audio engine */
//v8::Handle<v8::Value> Audio::AudioEngine::NewInstance(const v8::Arguments& args) {

NAN_METHOD(Audio::AudioEngine::NewInstance){
    NanScope();
	//HandleScope scope;

	unsigned argc = args.Length();

	if( argc > 2 )
		argc = 2;

	Handle<Value>* argv = new Handle<Value>[argc];

	argv[0] = args[0];
	if( argc > 1 )
		argv[1] = args[1];

	//Local<Object> instance = constructor->NewInstance( argc, argv );
	Local<Object> instance = NanNew(constructor)->NewInstance(argc, argv);
	//Local<Object> instance = constructor->NewInstance(argc, argv);

	NanReturnValue( instance );
} // end AudioEngine::NewInstance()

//////////////////////////////////////////////////////////////////////////////
/*! Create a v8 object */
//v8::Handle<v8::Value> Audio::AudioEngine::New( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::New){
    NanScope();
	//HandleScope scope;

	Local<Object> options;

	if( args.Length() > 0 ) {
		if( !args[0]->IsObject() )
            return NanThrowTypeError("First argument must be an object.");
		else
			options = Local<Object>::Cast( args[0] );
	} else {
		options = NanNew<Object>();
	}

	AudioEngine* pEngine = new AudioEngine( options );
	pEngine->Wrap( args.This() );

	NanReturnValue( args.This() );
} // end AudioEngine::New()


//////////////////////////////////////////////////////////////////////////////
/*! Write samples to the current audio device */
//v8::Handle<v8::Value> Audio::AudioEngine::write( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::write){
    NanScope();
	//HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	if (args.Length() > 1 || !args[0]->IsArray()){
        return NanThrowTypeError("First argument should be an array.");
	}

	uv_mutex_lock( &pEngine->m_mutex );
	pEngine->queueOutputBuffer( Local<Array>::Cast(args[0]) );
	uv_mutex_unlock( &pEngine->m_mutex );

	Handle<Boolean> result = NanNew<Boolean>( false );

	NanReturnValue( result );
} // end AudioEngine::Write()

//////////////////////////////////////////////////////////////////////////////
/*! Checks if the current audio buffer has been fed to Port Audio */
//v8::Handle<v8::Value> Audio::AudioEngine::isBufferEmpty( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::isBufferEmpty){
    NanScope();
	//HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	uv_mutex_lock( &pEngine->m_mutex );
	Handle<Boolean> isEmpty = NanNew<Boolean>(pEngine->m_uNumCachedOutputSamples[pEngine->m_uCurrentWriteBuffer] == 0);
	uv_mutex_unlock( &pEngine->m_mutex );
	NanReturnValue( isEmpty );
} // end AudioEngine::isBufferEmpty()

//////////////////////////////////////////////////////////////////////////////
/*! Read samples from the current audio device */
//v8::Handle<v8::Value> Audio::AudioEngine::read( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::read){
    NanScope();
	//HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	if (pEngine->m_bReadMicrophone) {
		Pa_ReadStream( pEngine->m_pPaStream, pEngine->m_cachedInputSampleBlock, pEngine->m_uSamplesPerBuffer );
	}

	Handle<Array> input = pEngine->getInputBuffer();

	NanReturnValue( input );
} // end AudioEngine::Read()


//////////////////////////////////////////////////////////////////////////////
/*! Returns whether the PortAudio stream is active */
//v8::Handle<v8::Value> Audio::AudioEngine::isActive( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::isActive){
    NanScope();
	//HandleScope scope;

	AudioEngine* pEngine = AudioEngine::Unwrap<AudioEngine>( args.This() );

	if( Pa_IsStreamActive(pEngine->m_pPaStream) )
		NanReturnValue( NanNew<Boolean>(true) );
	else
		NanReturnValue( NanNew<Boolean>(false) );
} // end AudioEngine::IsActive()


//////////////////////////////////////////////////////////////////////////////
/*! Get the name of an audio device with a given ID number */
//v8::Handle<v8::Value> Audio::AudioEngine::getDeviceName( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::getDeviceName){
    NanScope();
	//HandleScope scope;

	if( !args[0]->IsNumber() ) {
		return  NanThrowTypeError("getDeviceName() requires a device index");
	}

	Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

	const PaDeviceInfo* pDeviceInfo = Pa_GetDeviceInfo( (PaDeviceIndex)deviceIndex->NumberValue() );

	NanReturnValue( NanNew<String>(pDeviceInfo->name) );
} // end AudioEngine::GetDeviceName()


//////////////////////////////////////////////////////////////////////////////
/*! Get the number of available devices */
//v8::Handle<v8::Value> Audio::AudioEngine::getNumDevices( const v8::Arguments& args ) {
NAN_METHOD(Audio::AudioEngine::getNumDevices){
    NanScope();
	//HandleScope scope;

	int deviceCount = Pa_GetDeviceCount();

	NanReturnValue( NanNew<Number>(deviceCount) );
} // end AudioEngine::GetNumDevices()


//////////////////////////////////////////////////////////////////////////////
/*! Closes and reopens the PortAudio stream */
void Audio::AudioEngine::restartStream() {
	PaError error;

	// Stop the audio stream
	error = Pa_StopStream( m_pPaStream );

	if( error != paNoError )
		NanThrowTypeError("Failed to stop audio stream");

	// Close the audio stream
	error = Pa_CloseStream( m_pPaStream );

	if( error != paNoError )
		NanThrowTypeError("Failed to close audio stream");

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
		NanThrowTypeError("Failed to open audio stream :(");
	}

	// Start the audio stream
	error = Pa_StartStream( m_pPaStream );

	if( error != paNoError )
		NanThrowTypeError("Failed to start audio stream");

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
