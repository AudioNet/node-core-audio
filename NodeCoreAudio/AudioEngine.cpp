/**
    AudioEngine.cpp: Core audio functionality
 */
// DevStudio!namespace Audio DevStudio!class AudioEngine


#include "AudioEngine.h"
#include "portaudio.h"
#include <v8.h>
#include <node_internals.h>
#include <node_object_wrap.h>
#include <pthread.h>
#include <stdlib.h>
using namespace v8;

Persistent<Function> Audio::AudioEngine::constructor;

/**
 * The constructor.
 */
Audio::AudioEngine::AudioEngine( Local<Function>& callback, Local<Object> options, bool withThread ) :
    audioJsCallback(Persistent<Function>::New(callback)){
    
    PaError openStreamErr;
    paStream = NULL;

    //set defaults
    inputChannels = 2;
    outputChannels = 2;
    framesPerBuffer = DEFAULT_FRAMES_PER_BUFFER;
    cachedOutputSamplesLength = 0;
    sampleFormat = 1;
    interleaved = false;

    sampleRate = DEFAULT_SAMPLE_RATE;

    cachedInputSampleBlock = NULL;
    cachedOutputSampleBlock = NULL;

    inputOverflowed = 0;
    outputUnderflowed = 0;

    // Initialize our audio core
    PaError initErr = Pa_Initialize();

    inputDevice = Pa_GetDefaultInputDevice();
    if (inputDevice == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default input device")) );
    };

    outputDevice = Pa_GetDefaultOutputDevice();
    if (outputDevice == paNoDevice) {
        ThrowException( Exception::TypeError(String::New("Error: No default output device")) );
    }

    if( initErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to initialize audio engine")) );

    applyOptions(options);

    
    fprintf(stderr, "input :%d\n", inputDevice);
    fprintf(stderr, "output :%d\n", outputDevice);
    fprintf(stderr, "rate :%d\n", sampleRate);
    fprintf(stderr, "format :%d\n", sampleFormat);
    fprintf(stderr, "size :%ld\n", sizeof(float));
    fprintf(stderr, "inputChannels :%d\n", inputChannels);
    fprintf(stderr, "outputChannels :%d\n", outputChannels);
    fprintf(stderr, "interleaved :%d\n", interleaved);
    

    pthread_mutex_init(&callerThreadSamplesAccess, NULL);

    // Open an audio I/O stream. 
    openStreamErr = Pa_OpenStream(  &paStream,
                                    &inputParameters,
                                    &outputParameters,
                                    sampleRate,
                                    framesPerBuffer,
                                    paClipOff,
                                    NULL,
                                    NULL );

    if( openStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream")) );

    // Start the audio stream
    PaError startStreamErr = Pa_StartStream( paStream );

    if( startStreamErr != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

    if (withThread)
        pthread_create(&ptStreamThread, NULL, Audio::AudioEngine::streamThread, this);

} // end Constructor


/**
 * GetOptions function for v8 getOptions.
 */
v8::Handle<v8::Value> Audio::AudioEngine::GetOptions(const v8::Arguments& args){
    HandleScope scope;

    Local<Object> options;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );
    
    options->Set(String::New("inputChannels"), Number::New(engine->inputChannels));
    options->Set(String::New("outputChannels"), Number::New(engine->outputChannels));

    options->Set(String::New("inputDevice"), Number::New(engine->inputDevice));
    options->Set(String::New("outputDevice"), Number::New(engine->outputDevice));

    options->Set(String::New("sampleRate"), Number::New(engine->sampleRate));
    options->Set(String::New("sampleFormat"), Number::New(engine->sampleFormat));

    options->Set(String::New("framesPerBuffer"), Number::New(engine->framesPerBuffer));
    options->Set(String::New("interleaved"), Boolean::New(engine->interleaved));
    
    return scope.Close(options);

} // end GetOptions


/**
 * SetOptions function for v8 setOptions.
 */
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

} // end SetOptions


/**
 * Applys the options to the parameter objects and restarts the stream.
 */
void Audio::AudioEngine::applyOptions( Local<Object> options ){

    if (options->HasOwnProperty(String::New("inputChannels")))
        inputChannels = options->Get(String::New("inputChannels"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("outputChannels")))
        outputChannels = options->Get(String::New("outputChannels"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("framesPerBuffer")))
        framesPerBuffer = options->Get(String::New("framesPerBuffer"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("sampleFormat"))){
        switch (options->Get(String::New("sampleFormat"))->ToInteger()->Value()){
            case 1: sampleFormat = paFloat32; sampleSize = 4; break;
            case 2: sampleFormat = paInt32; sampleSize = 4; break;
            case 4: sampleFormat = paInt24; sampleSize = 3; break;
            case 8: sampleFormat = paInt16; sampleSize = 2; break;
            case 10: sampleFormat = paInt8; sampleSize = 1; break;
            case 20: sampleFormat = paUInt8; sampleSize = 1; break;
        }
    }

    if (options->HasOwnProperty(String::New("inputDevice")))
        inputDevice = options->Get(String::New("inputDevice"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("outputDevice")))
        outputDevice = options->Get(String::New("outputDevice"))->ToInteger()->Value();

    if (options->HasOwnProperty(String::New("interleaved")))
        interleaved = options->Get(String::New("interleaved"))->ToBoolean()->Value();


    // input
    inputParameters.device = inputDevice;
    inputParameters.channelCount = inputChannels;
    inputParameters.sampleFormat = sampleFormat;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    if (cachedInputSampleBlock != NULL)
        free(cachedInputSampleBlock);

    cachedInputSampleBlock = (char *)malloc(framesPerBuffer * inputChannels * sampleSize);

    // Stereo output
    outputParameters.device = outputDevice;
    outputParameters.channelCount = outputChannels;
    outputParameters.sampleFormat = sampleFormat;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if (cachedOutputSampleBlock != NULL)
        free(cachedOutputSampleBlock);

    cachedOutputSampleBlock = (char *)malloc(framesPerBuffer * outputChannels * sampleSize);

    if (paStream != NULL && Pa_IsStreamActive(paStream))
        restartStream();

} // end applyOptions


/**
 * Prepares the input/output buffers and calls the javascript callback.
 */
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

    if (result->IsArray()){
        engine->queueOutputBuffer(result);
    }
    
    pthread_mutex_unlock(&engine->callerThreadSamplesAccess);
    
    return 0;
} // end callCallback


/**
 * Returns an v8 Array based on the cachedInput.
 */
Handle<Array> Audio::AudioEngine::InputToArray(){
    int iSample = 0, iChannel = 0;
    HandleScope scope;

    Handle<Array> inputBuffer;

    Handle<Number> sample;

    if (interleaved){
        inputBuffer = Array::New( inputChannels * framesPerBuffer );

        while(iSample < framesPerBuffer*inputChannels){

            inputBuffer->Set(
                iSample,
                getSample(iSample)
            );

            iSample++;
        }

    } else {
        inputBuffer = Array::New( inputChannels );

        for (iChannel=0;iChannel<inputChannels;iChannel++)
            inputBuffer->Set(iChannel, Array::New(framesPerBuffer));

        iChannel = -1;
        while(iSample < framesPerBuffer*inputChannels){

            if (iSample%inputChannels == 0)
                iChannel++;

            inputBuffer->Get(iSample%inputChannels)->ToObject()->Set(
                iChannel,
                getSample(iSample)
            );

            iSample++;
        }

    }

    return inputBuffer;
} // end InputToArray

/**
 * Returns the samples recorded from the soundcard as v8 Number.
 */
Handle<Number> Audio::AudioEngine::getSample(int position){
    HandleScope scope;

    Handle<Number> sample;

    switch (sampleFormat){
        case paFloat32:
            sample = Number::New(((float*)cachedInputSampleBlock)[position]); break;
        case paInt32:
            sample = Integer::New(((int*)cachedInputSampleBlock)[position]); break;
        case paInt24:
            sample = Integer::New(
                (cachedInputSampleBlock[3*position + 0] << 16)
              + (cachedInputSampleBlock[3*position + 1] << 8)
              + (cachedInputSampleBlock[3*position + 2])
            );
            break;
        case paInt16:
            sample = Integer::New(((int16_t*)cachedInputSampleBlock)[position]); break;
        default:
            sample = Integer::New(cachedInputSampleBlock[position]*-1); break;
    }

    return sample;
} // end getSample


/**
 * Sets a v8 sample value to a position for output.
 */
void Audio::AudioEngine::setSample(int position, Handle<Value> sample){

    int temp;
    switch (sampleFormat){
        case paFloat32:
            ((float *)cachedOutputSampleBlock)[position] = (float)sample->NumberValue();
            break;
        case paInt32:
            ((int *)cachedOutputSampleBlock)[position] = (int)sample->NumberValue();
            break;
        case paInt24:

            temp = (int)sample->NumberValue();
            cachedOutputSampleBlock[3*position + 0] = temp >> 16;
            cachedOutputSampleBlock[3*position + 1] = temp >> 8;
            cachedOutputSampleBlock[3*position + 2] = temp;

            break;
        case paInt16:
            ((int16_t *)cachedOutputSampleBlock)[position] = (int16_t)sample->NumberValue();
            break;
        default:
            cachedOutputSampleBlock[position] = (char)sample->NumberValue();
            break;
    }
} // end setSample


/**
 * Queues the buffer for being used in the streamThread.
 */
void Audio::AudioEngine::queueOutputBuffer(Handle<Array> result){
    int iSample = 0, iChannel = -1;
    HandleScope scope;

    cachedOutputSamplesLength = 0;

    //int *out = (int*)cachedOutputSampleBlock;

    if (interleaved){

        while(iSample < framesPerBuffer*outputChannels){

            setSample(iSample, result->Get(iSample));
            iSample++;
        }

        cachedOutputSamplesLength = result->Length()/outputChannels;

    } else {

        Handle<Array> item;

        while(iSample < framesPerBuffer*outputChannels){

            if (iSample%outputChannels == 0)
                iChannel++;

            item = Handle<Array>::Cast(result->Get(iSample%outputChannels));

            if (item->IsArray()){
                if (item->Length() > cachedOutputSamplesLength)
                    cachedOutputSamplesLength = item->Length();
                
                setSample(iSample, item->Get(iChannel));
            }

            iSample++;
        }

    }

} // end queueOutputBuffer


/**
 * The main thread that writes/reads the buffer from the soundcard stream.
 * If the user doesn't apply a javascript callback, this thread won't start.
 */
void* Audio::AudioEngine::streamThread(void *pEngine){

    AudioEngine* engine = ((AudioEngine*)pEngine);
    PaError err;

    while(1){

        err = Pa_ReadStream(engine->paStream, engine->cachedInputSampleBlock, engine->framesPerBuffer);

        engine->inputOverflowed = (err != paNoError);

        pthread_mutex_trylock(&engine->callerThreadSamplesAccess);
        eio_nop(EIO_PRI_DEFAULT, callCallback, pEngine); //call into the v8 thread
        pthread_mutex_lock(&engine->callerThreadSamplesAccess); //wait, untill js call is done

        if (engine->cachedOutputSamplesLength > 0){
            err = Pa_WriteStream(engine->paStream, engine->cachedOutputSampleBlock, engine->cachedOutputSamplesLength);
            engine->outputUnderflowed = (err != paNoError);
            engine->cachedOutputSamplesLength = 0;
        }
    }
} // end streamThread


/**
 * The node.js instantiation function.
 */
void Audio::AudioEngine::Init(v8::Handle<v8::Object> target) {
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
} // end Init()


/**
 * Instantiate from instance factory.
 */
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
} // end NewInstance()


/**
 * The V8 new operator.
 */
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
} // end New()


/**
 * Writes the buffer to the soundcard. This function doesn't return until the entire buffer has been consumed.
 */
v8::Handle<v8::Value> Audio::AudioEngine::Write( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    if (args.Length() > 1 || !args[0]->IsArray()){
        return ThrowException( Exception::TypeError(String::New("First argument should be an array.")) );
    }

    engine->queueOutputBuffer(Local<Array>::Cast( args[0] ));

    Handle<Boolean> result = Boolean::New(false);

    if (engine->cachedOutputSamplesLength > 0){
        if (!Pa_WriteStream(engine->paStream, engine->cachedOutputSampleBlock, engine->cachedOutputSamplesLength))
            result = Boolean::New(true);
        engine->cachedOutputSamplesLength = 0;
    }

    return scope.Close( result );
} // end Write


/**
 * Reads the inut buffer from the soundcard. The function doesn't return until the entire buffer has been filled.
 */
v8::Handle<v8::Value> Audio::AudioEngine::Read( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    Pa_ReadStream(engine->paStream, engine->cachedInputSampleBlock, engine->framesPerBuffer);

    Handle<Array> input = engine->InputToArray();
    
    return scope.Close( input );
} // end Read


/**
 * Returns whether the audio stream active.
 */
v8::Handle<v8::Value> Audio::AudioEngine::IsActive( const v8::Arguments& args ) {
    HandleScope scope;

    AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );
    
    if (Pa_IsStreamActive(engine->paStream))
        return scope.Close( Boolean::New(true) );
    else
        return scope.Close( Boolean::New(false) );
} // end IsActive()


/**
 * Returns the name of the device corresponding to a given ID number.
 */
v8::Handle<v8::Value> Audio::AudioEngine::GetDeviceName( const v8::Arguments& args ) {
    HandleScope scope;

    if( !args[0]->IsNumber() ) {
        return ThrowException( Exception::TypeError(String::New("getDeviceName() requires a device index")) );
    }

    Local<Number> deviceIndex = Local<Number>::Cast( args[0] );

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( (PaDeviceIndex)deviceIndex->NumberValue() );

    return scope.Close( String::New(deviceInfo->name) );
} // end GetDeviceName()


/**
 * Returns the number of devices we have available.
 */
v8::Handle<v8::Value> Audio::AudioEngine::GetNumDevices( const v8::Arguments& args ) {
    HandleScope scope;

    //AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>( args.This() );

    int deviceCount = Pa_GetDeviceCount();

    return scope.Close( Number::New(deviceCount) );
} // end GetNumDevices()


/**
 * Stops and restarts our audio stream.
 */
void Audio::AudioEngine::restartStream() {
    Pa_StopStream( paStream );
    Pa_CloseStream( paStream );

    PaError err;

    // Open an audio I/O stream. 
    err = Pa_OpenStream(  &paStream,
        &inputParameters,
        &outputParameters,
        sampleRate,
        framesPerBuffer,
        paClipOff,              // We won't output out of range samples so don't bother clipping them
        NULL,
        NULL );

    if( err != paNoError ) {
        ThrowException( Exception::TypeError(String::New("Failed to open audio stream :(")) );
    }

    err = Pa_StartStream( paStream );

    if( err != paNoError ) 
        ThrowException( Exception::TypeError(String::New("Failed to start audio stream")) );

} // end restartStream()


/**
 * Wraps a handle into an object.
 */
void Audio::AudioEngine::wrapObject( v8::Handle<v8::Object> object ) {
    ObjectWrap::Wrap( object );
} // end wrapObject()


/**
 * Destructor.
 */
Audio::AudioEngine::~AudioEngine() {
    PaError err = Pa_Terminate();
    if( err != paNoError )
        fprintf( stderr, "PortAudio error: %s\n", Pa_GetErrorText( err ) );

} // end ~AudioEngine()
