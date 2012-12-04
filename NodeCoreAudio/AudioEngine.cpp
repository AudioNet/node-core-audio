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
#include <pthread.h>
using namespace v8;

Persistent<Function> Audio::AudioEngine::constructor;

v8::Handle<v8::Value> Audio::AudioEngine::SetOptions(const v8::Arguments& args){
    HandleScope scope;

    Local<Object> options;

    if (args.Length() > 0){
        if (!args[0]->IsObject()) {
            return ThrowException( Exception::TypeError(String::New("First argument should be an object.")) );
        }
        options = Local<Object>::Cast( args[0] );
    } else {
        return Exception::TypeError(String::New("First argument does not exist."));
    }

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );
    engine->applyOptions(options);
    
    return scope.Close(Undefined());

}

void Audio::AudioEngine::applyOptions( Local<Object> options ){

    if (options->HasOwnProperty(String::New("inputChannels")))
        inputChannels = options->Get(String::New("inputChannels"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("outputChannels")))
        outputChannels = options->Get(String::New("outputChannels"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("framesPerBuffer")))
        framesPerBuffer = options->Get(String::New("framesPerBuffer"))->ToInteger()->Value();

    // Setup default input device
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default input device")) );
    }

    // input
    inputParameters.channelCount = inputChannels;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    if (cachedInputSamples != NULL)
        free(cachedInputSamples);

    cachedInputSamples = (float *)malloc(sizeof(float) * framesPerBuffer * inputChannels);

    // Setup default output device
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default output device")) );
    }

    // Stereo output
    outputParameters.channelCount = outputChannels;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if (cachedOutputSamples != NULL)
        free(cachedOutputSamples);

    cachedOutputSamples = (float *)malloc((sizeof(float) * framesPerBuffer) * outputChannels);

    if (paStream != NULL && Pa_IsStreamActive(paStream))
        restartStream();

}

//////////////////////////////////////////////////////////////////////////////
/*! Initalize */
Audio::AudioEngine::AudioEngine( Local<Function>& callback, Local<Object> options, bool withThread ) :
    audioJsCallback(Persistent<Function>::New(callback)){
    
    PaError openStreamErr;
    paStream = NULL;

    //set defaults
    inputChannels = 2;
    outputChannels = 2;
    framesPerBuffer = DEFAULT_FRAMES_PER_BUFFER;
    cachedOutputSamplesLength = 0;

    cachedInputSamples = NULL;
    cachedOutputSamples = NULL;

    inputOverflowed = 0;
    outputUnderflowed = 0;

    // Initialize our audio core
    PaError initErr = Pa_Initialize();

    if( initErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to initialize audio engine")) );

    applyOptions(options);

    pthread_mutex_init(&callerThreadSamplesAccess, NULL);

    pthread_t ptCallerThread;

    // Open an audio I/O stream. 
    openStreamErr = Pa_OpenStream(  &paStream,
                                    &inputParameters,
                                    &outputParameters,
                                    SAMPLE_RATE,
                                    framesPerBuffer,
                                    paClipOff,              // We won't output out of range samples so don't bother clipping them
                                    NULL,   // this is your callback function
                                    NULL );                 // This is a pointer that will be passed to your callback*/

    if( openStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream")) );

    // Start the audio stream
    PaError startStreamErr = Pa_StartStream( paStream );

    if( startStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

    if (withThread)
        pthread_create(&ptCallerThread, NULL, Audio::AudioEngine::streamThread, this);

} // end AudioEngine::AudioEngine()


int Audio::AudioEngine::callCallback(eio_req *req){
    HandleScope scope;

    AudioEngine* engine = ((AudioEngine*)req->data);

    Handle<Array> inputBuffer = engine->InputToArray();

    Local<Value> argv[3] = {
        Local<Value>::New(inputBuffer),
        Local<Value>::New(Boolean::New(engine->inputOverflowed)),
        Local<Value>::New(Boolean::New(engine->outputUnderflowed))
    };

    Handle<Array> result = Handle<Array>::Cast(engine->audioJsCallback->Call(engine->audioJsCallback, 3, argv));

    // copy output
    if (result->IsArray()){
        engine->queueOutputBuffer(result);
    }
    // end copy output
    
    pthread_mutex_unlock(&engine->callerThreadSamplesAccess);
    
    return 0;
}

Handle<Array> Audio::AudioEngine::InputToArray(){
    int iSample = 0, iChannel = 0;

    Handle<Array> inputBuffer = Array::New( inputChannels );

    for (iChannel=0;iChannel<inputChannels;iChannel++)
        inputBuffer->Set(iChannel, Array::New(framesPerBuffer));

    iChannel = -1;
    while(iSample < framesPerBuffer*inputChannels){

        if (iSample%inputChannels == 0)
            iChannel++;

        inputBuffer->Get(iSample%inputChannels)->ToObject()->Set(
            iChannel,
            v8::Number::New(cachedInputSamples[iSample])
        );

        iSample++;
    }

    return inputBuffer;
}

void Audio::AudioEngine::queueOutputBuffer(Handle<Array> result){
    int iSample = 0, iChannel = -1;

    Handle<Array> item;
    cachedOutputSamplesLength = 0;

    while(iSample < framesPerBuffer*outputChannels){
        
        if (iSample%outputChannels == 0)
            iChannel++;

        item = Handle<Array>::Cast(result->Get(iSample%outputChannels));

        if (item->IsArray()){
            if (item->Length() > cachedOutputSamplesLength)
                cachedOutputSamplesLength = item->Length();

            cachedOutputSamples[iSample] = (float)item->Get(iChannel)->NumberValue();
        }

        iSample++;
    }

}


void* Audio::AudioEngine::streamThread(void *pEngine){

    AudioEngine* engine = ((AudioEngine*)pEngine);
    PaError err;

    while(1){

        err = Pa_ReadStream(engine->paStream, engine->cachedInputSamples, engine->framesPerBuffer);

        engine->inputOverflowed = (err != paNoError);

        //Pa_WriteStream(engine->paStream, engine->cachedInputSamples, engine->framesPerBuffer);

        pthread_mutex_trylock(&engine->callerThreadSamplesAccess);
        eio_nop(EIO_PRI_DEFAULT, callCallback, pEngine); //call into the v8 thread
        pthread_mutex_lock(&engine->callerThreadSamplesAccess); //wait, untill js call is done

        if (engine->cachedOutputSamplesLength > 0){
            err = Pa_WriteStream(engine->paStream, engine->cachedOutputSamples, engine->cachedOutputSamplesLength);
            engine->outputUnderflowed = (err != paNoError);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
/*! Our node.js instantiation function */
void Audio::AudioEngine::Init(v8::Handle<v8::Object> target) {
    // Prepare constructor template
    Local<FunctionTemplate> functionTemplate = FunctionTemplate::New (Audio::AudioEngine::New );
    functionTemplate->SetClassName( String::NewSymbol("AudioEngine") );
    functionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );

    // Get
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("isActive"), FunctionTemplate::New(Audio::AudioEngine::IsActive)->GetFunction() );
    //functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("processIfNewData"), FunctionTemplate::New(Audio::AudioEngine::ProcessIfNewData)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getSampleRate"), FunctionTemplate::New(Audio::AudioEngine::GetSampleRate)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getInputDeviceIndex"), FunctionTemplate::New(Audio::AudioEngine::GetInputDeviceIndex)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getOutputDeviceIndex"), FunctionTemplate::New(Audio::AudioEngine::GetOutputDeviceIndex)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getDeviceName"), FunctionTemplate::New(Audio::AudioEngine::GetDeviceName)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getNumDevices"), FunctionTemplate::New(Audio::AudioEngine::GetNumDevices)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getNumInputChannels"), FunctionTemplate::New(Audio::AudioEngine::GetNumInputChannels)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("getNumOutputChannels"), FunctionTemplate::New(Audio::AudioEngine::GetNumOutputChannels)->GetFunction() );

    // Set
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("setInputDevice"), FunctionTemplate::New(Audio::AudioEngine::SetInputDevice)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("setOutputDevice"), FunctionTemplate::New(Audio::AudioEngine::SetOutputDevice)->GetFunction() );

    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("setOptions"), FunctionTemplate::New(Audio::AudioEngine::SetOptions)->GetFunction() );
    functionTemplate->PrototypeTemplate()->Set( String::NewSymbol("write"), FunctionTemplate::New(Audio::AudioEngine::Write)->GetFunction() );

    constructor = Persistent<Function>::New( functionTemplate->GetFunction() );
} // end AudioEngine::Init()


//////////////////////////////////////////////////////////////////////////////
/*! Instantiate from instance factory */
v8::Handle<v8::Value> Audio::AudioEngine::NewInstance( const v8::Arguments& args ) {
    HandleScope scope;

    unsigned argc = args.Length();
    
    if (argc > 2) argc = 2;

    Handle<Value> *argv = new Handle<Value>[argc];

    argv[0] = args[0];
    if (argc > 1)
        argv[1] = args[1];

    Local<Object> instance = constructor->NewInstance( argc, argv );

    return scope.Close(instance);
} // end AudioEngine::NewInstance()


//////////////////////////////////////////////////////////////////////////////
/*! Our V8 new operator */
v8::Handle<v8::Value> Audio::AudioEngine::New( const v8::Arguments& args ) {
    HandleScope scope;

    Local<Function> callback;
    Local<Object> options;
    bool withThread = false;

    if (args.Length() > 0) {
        if (!args[0]->IsObject())
            return ThrowException( Exception::TypeError(String::New("First argument must be an object.")) );
        else
            options = Local<Object>::Cast( args[0] );
    } else {
        options = Object::New();
    }
    
    if (args.Length() > 1){
        if (args[1]->IsFunction()) {
            callback = Local<Function>::Cast( args[1] );
            withThread = true;
        }
    }

    AudioEngine* engine = new AudioEngine( callback, options, withThread );
    engine->Wrap( args.This() );

    return scope.Close( args.This() );
} // end AudioEngine::New()


v8::Handle<v8::Value> Audio::AudioEngine::Write( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    if (args.Length() > 1 || !args[0]->IsArray()){
        return ThrowException( Exception::TypeError(String::New("First argument should be an array.")) );
    }

    engine->queueOutputBuffer(Local<Array>::Cast( args[0] ));

    return scope.Close( args.This() );
}


v8::Handle<v8::Value> Audio::AudioEngine::Read( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    Handle<Array> input = engine->InputToArray();
    
    return scope.Close( input );
}

//////////////////////////////////////////////////////////////////////////////
/*! Returns whether the audio stream active */
v8::Handle<v8::Value> Audio::AudioEngine::IsActive( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );
    
    if (Pa_IsStreamActive(engine->paStream))
        return scope.Close( Boolean::New(true) );
    else
        return scope.Close( Boolean::New(false) );
} // end AudioEngine::IsActive()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the sample rate */
v8::Handle<v8::Value> Audio::AudioEngine::GetSampleRate( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    const PaStreamInfo* streamInfo = Pa_GetStreamInfo(engine->paStream);

    return scope.Close( Number::New(streamInfo->sampleRate) );
} // end AudioEngine::GetSampleRate()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the number of input channels */
v8::Handle<v8::Value> Audio::AudioEngine::GetNumInputChannels( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    return scope.Close( Number::New(engine->inputParameters.channelCount) );
} // end AudioEngine::GetNumInputChannels()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the index of the input device */
v8::Handle<v8::Value> Audio::AudioEngine::GetInputDeviceIndex( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    return scope.Close( Number::New(engine->inputParameters.device) );
} // end AudioEngine::GetInputDeviceIndex()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the index of the output device */
v8::Handle<v8::Value> Audio::AudioEngine::GetOutputDeviceIndex( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    return scope.Close( Number::New(engine->outputParameters.device) );
} // end AudioEngine::GetOutputDeviceIndex()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the name of the device corresponding to a given ID number */
v8::Handle<v8::Value> Audio::AudioEngine::GetDeviceName( const v8::Arguments& args ) {
    HandleScope scope;

    if( !args[0]->IsNumber() ) {
        return ThrowException( Exception::TypeError(String::New("getDeviceName() requires a device index")) );
    }

    //AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( (PaDeviceIndex)deviceIndex->NumberValue() );

    return scope.Close( String::New(deviceInfo->name) );
} // end AudioEngine::GetDeviceName()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the number of output channels */
v8::Handle<v8::Value> Audio::AudioEngine::GetNumOutputChannels( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    return scope.Close( Number::New(engine->outputParameters.channelCount) );
} // end AudioEngine::GetNumOutputChannels()


//////////////////////////////////////////////////////////////////////////////
/*! Returns the number of devices we have available */
v8::Handle<v8::Value> Audio::AudioEngine::GetNumDevices( const v8::Arguments& args ) {
    HandleScope scope;

    //AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    int deviceCount = Pa_GetDeviceCount();

    return scope.Close( Number::New(deviceCount) );
} // end AudioEngine::GetNumDevices()


//////////////////////////////////////////////////////////////////////////////
/*! Changes the input device */
v8::Handle<v8::Value> Audio::AudioEngine::SetInputDevice( const v8::Arguments& args ) {
    HandleScope scope;

    // Validate the input args
    if( !args[0]->IsNumber() ) {
        return ThrowException( Exception::TypeError(String::New("setInputDevice() requires a device index")) );
    }

    Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    // Set the new input device
    engine->inputParameters.device = deviceIndex->NumberValue();

    // Restart the audio stream
    engine->restartStream();

    return scope.Close( Undefined() );
} // end AudioEngine::SetInputDevice()


//////////////////////////////////////////////////////////////////////////////
/*! Changes the output device */
v8::Handle<v8::Value> Audio::AudioEngine::SetOutputDevice( const v8::Arguments& args ) {
    HandleScope scope;

    // Validate the output args
    if( !args[0]->IsNumber() ) {
        return ThrowException( Exception::TypeError(String::New("setOutputDevice() requires a device index")) );
    }

    Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    // Set the new output device
    engine->outputParameters.device = deviceIndex->NumberValue();

    // Restart the audio stream
    engine->restartStream();

    return scope.Close( Undefined() );
} // end AudioEngine::SetOutputDevice()


//////////////////////////////////////////////////////////////////////////////
/*! Stops and restarts our audio stream */
void Audio::AudioEngine::restartStream() {
    Pa_StopStream( paStream );
    Pa_CloseStream( paStream );

    PaError err;

    // Open an audio I/O stream. 
    err = Pa_OpenStream(  &paStream,
        &inputParameters,
        &outputParameters,
        SAMPLE_RATE,
        framesPerBuffer,
        paClipOff,              // We won't output out of range samples so don't bother clipping them
        NULL,   // this is your callback function
        NULL );                 // This is a pointer that will be passed to your callback*/

    if( err != paNoError ) {
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream :(")) );
    }

    err = Pa_StartStream( paStream );

    if( err != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

} // end AudioEngine::restartStream()

//////////////////////////////////////////////////////////////////////////////
/*! Wraps a handle into an object */
void Audio::AudioEngine::wrapObject( v8::Handle<v8::Object> object ) {
    ObjectWrap::Wrap( object );
} // end AudioEngine::wrapObject()


//////////////////////////////////////////////////////////////////////////////
/*! Destructor */
Audio::AudioEngine::~AudioEngine() {
    PaError err = Pa_Terminate();
    if( err != paNoError )
        fprintf( stderr, "PortAudio error: %s\n", Pa_GetErrorText( err ) );
/*
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, 2);

    fprintf(stderr, "!!DESTRUCTOR!!!\n");

    ThrowException( Exception::TypeError(String::New("Being destructed")) );*/

} // end AudioEngine::~AudioEngine()
