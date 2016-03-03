/**
 * Core audio functionality
 * @file: AudioEngine.cpp
 */
#include "AudioEngine.h"

using namespace v8;
using namespace std;

Persistent<Function> Audio::AudioEngine::constructor;

static void do_work( void* arg ) {
	Audio::AudioEngine* engine = (Audio::AudioEngine*)arg;
	engine->RunAudioLoop();
}

/**
 * Initialize AudioEngine node object. This helper defines every methods,
 * properties. Futhermore it launches PortAudio initialization.
 */
NAN_MODULE_INIT(Audio::AudioEngine::Init) {
	Local<FunctionTemplate> aet;

	// First initialize PortAudio
	Audio::AudioEngine::initializePortAudio();

	// Prepare AudioEngine template (aet)
	aet = Nan::New<FunctionTemplate>(Audio::AudioEngine::New);
	aet->SetClassName(Nan::New<String>(AUDIO_ENGINE_CLASSNAME).ToLocalChecked());
	aet->InstanceTemplate()->SetInternalFieldCount( 1 );

	// Static methods
	Nan::SetMethod(aet, "getDeviceName", Audio::AudioEngine::getDeviceName);
	Nan::SetMethod(aet, "getNumDevices", Audio::AudioEngine::getNumDevices);

	// Class methods
	// NOTE: isActive and isBufferEmpty could be replaced by native getters
	Nan::SetPrototypeMethod(aet, "setOptions",    Audio::AudioEngine::setOptions);
	Nan::SetPrototypeMethod(aet, "getOptions",    Audio::AudioEngine::getOptions);
	Nan::SetPrototypeMethod(aet, "write",         Audio::AudioEngine::write);
	Nan::SetPrototypeMethod(aet, "read",          Audio::AudioEngine::read);
	Nan::SetPrototypeMethod(aet, "isBufferEmpty", Audio::AudioEngine::isBufferEmpty);
	Nan::SetPrototypeMethod(aet, "isActive",      Audio::AudioEngine::isActive);

	// TODO: Why ? What ?
	constructor.Reset(Isolate::GetCurrent(), aet->GetFunction());

	// Publish object/function as property of the module
	Nan::Set(target, Nan::New(AUDIO_ENGINE_CLASSNAME).ToLocalChecked(), Nan::GetFunction(aet).ToLocalChecked());
}

/**
 * Initialize PortAudio or throw a native node error.
 */
void Audio::AudioEngine::initializePortAudio() {
	if (Pa_Initialize() != paNoError) {
		Nan::ThrowTypeError("Failed to initialize audio engine");
	}
}

/**
 * Create a native C++ AudioEngine and wrap it around a v8 object.
 */
void Audio::AudioEngine::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Local<Object> options;
	AudioEngine* engine;

	// If an argument is specified, it MUST be an object containing options
	// otherwise create an empty object
	if (info.Length() > 0) {
		if (!info[0]->IsObject()) {
			return Nan::ThrowTypeError("First argument must be an object.");
		}

		options = Local<Object>::Cast(info[0]);
	} else {
		options = Nan::New<Object>();
	}

	// Create native C++ object and wrap it
	engine = new AudioEngine(options);
	engine->Wrap(info.This());

	// Return newly created AudioEngine
	info.GetReturnValue().Set(info.This());
}

/**
 * Constructor a new native C++ AudioEngine object.
 * @constructor
 * @param {Local<Object>} options - Options of the AudioEngine.
 */
Audio::AudioEngine::AudioEngine(Local<Object> options) {
	// Reset stream to NULL by default
	stream = NULL;

	// Set default options
	setDefaultOptions();

	cachedInputSampleBlock            = NULL;
	cachedOutputSampleBlock           = NULL;
	cachedOutputSampleBlockForWriting = NULL;

	inputOverflowed    = 0;
	outputUnderflowed  = 0;
	currentWriteBuffer = 0;
	currentReadBuffer  = 0;

	applyOptions(options);
#ifdef DEBUG
	printOptions();
#endif
	initializeInputBuffer();
	initializeStream();

	uv_mutex_init(&m_mutex);
	uv_thread_create(&streamThread, do_work, (void*)this);
}

/**
 * Set default AudioEngine options and ensure we can define default input/output
 * audio devices.
 */
void Audio::AudioEngine::setDefaultOptions() {
	sampleSize             = 4;
	inputChannels          = 2;
	outputChannels         = 2;
	numCachedOutputSamples = 0;
	sampleFormat           = 1;
	sampleRate             = DEFAULT_SAMPLE_RATE;
	samplesPerBuffer       = DEFAULT_FRAMES_PER_BUFFER;
	numBuffers             = DEFAULT_NUM_BUFFERS;
	interleaved            = false;
	readMicrophone         = true;
	inputDevice            = Pa_GetDefaultInputDevice();
	outputDevice           = Pa_GetDefaultOutputDevice();

	// Ensure we defined default devices
	if(inputDevice == paNoDevice) {
		return Nan::ThrowTypeError("Error: No default input device");
	}

	if(outputDevice == paNoDevice) {
		return Nan::ThrowTypeError("Error: No default output device");
	}
}

/**
 * Initialize the PortAudio stream; it will attempt to open stream with given
 * options and start it.
 */
void Audio::AudioEngine::initializeStream() {
	PaError error;

#ifdef DEBUG
	fprintf(stderr, "Intializing stream...\n");
#endif

	// Open an audio I/O stream.
	error = Pa_OpenStream(&stream,
                          &streamInputParams,
                          &streamOutputParams,
                          sampleRate,
                          samplesPerBuffer,
                          paClipOff,
                          NULL,
                          NULL);
	if (error != paNoError) {
		return Nan::ThrowTypeError("Failed to open audio stream");
	}

	// Start the audio stream
	error = Pa_StartStream(stream);
	if (error != paNoError) {
		return Nan::ThrowTypeError("Failed to start audio stream");
	}
}

/**
 * Reinitialize the PortAudio stream; it will first remove previous one and then
 * call #initializeStream.
 */
void Audio::AudioEngine::reinitializeStream() {
	PaError error;

	// Stop the audio stream
	error = Pa_StopStream(stream);
	if(error != paNoError)
		Nan::ThrowTypeError("Failed to stop audio stream");

	// Close the audio stream
	error = Pa_CloseStream(stream);
	if( error != paNoError )
		Nan::ThrowTypeError("Failed to close audio stream");

	initializeStream();
}

/**
 * Create a two dimensional array as input buffer.
 * TODO: Should be replaced by real Node buffer.
 */
void Audio::AudioEngine::initializeInputBuffer() {
	// Foreach channel create a samples buffer
	inputBuffer = Nan::New<Array>(inputChannels);
	for(int channel = 0; channel < inputChannels; channel++) {
		inputBuffer->Set(channel, Nan::New<Array>(samplesPerBuffer));
	}
}

/**
 * Get AudioEngine options wrapper. It's creating a v8 object with options from
 * native C++ object.
 */
void Audio::AudioEngine::getOptions(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Local<Object> options = Nan::New<Object>();

	// Unwrap AudioEngine from v8 to C++ and dump options into a v8 object
	AudioEngine* engine = AudioEngine::Unwrap<AudioEngine>(info.This());
	Nan::Set(options, Nan::New<String>("inputChannels").ToLocalChecked(),   Nan::New<Number>(engine->inputChannels));
	Nan::Set(options, Nan::New<String>("outputChannels").ToLocalChecked(),  Nan::New<Number>(engine->outputChannels));
	Nan::Set(options, Nan::New<String>("inputDevice").ToLocalChecked(),     Nan::New<Number>(engine->inputDevice));
	Nan::Set(options, Nan::New<String>("outputDevice").ToLocalChecked(),    Nan::New<Number>(engine->outputDevice));
	Nan::Set(options, Nan::New<String>("sampleRate").ToLocalChecked(),      Nan::New<Number>(engine->sampleRate));
	Nan::Set(options, Nan::New<String>("sampleFormat").ToLocalChecked(),    Nan::New<Number>(engine->sampleFormat));
	Nan::Set(options, Nan::New<String>("framesPerBuffer").ToLocalChecked(), Nan::New<Number>(engine->samplesPerBuffer));
	Nan::Set(options, Nan::New<String>("numBuffers").ToLocalChecked(),      Nan::New<Number>(engine->numBuffers));
	Nan::Set(options, Nan::New<String>("interleaved").ToLocalChecked(),     Nan::New<Boolean>(engine->interleaved));
	Nan::Set(options, Nan::New<String>("useMicrophone").ToLocalChecked(),   Nan::New<Boolean>(engine->readMicrophone));

	info.GetReturnValue().Set(options);
}


/**
 * Set AudioEngine options wrapper. It's validating input parameter from v8
 * object and put them into native C++ object through #applyOptions.
 */
void Audio::AudioEngine::setOptions(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Local<Object> options;
	AudioEngine* engine;

	// We expect one argument, an object...
	if (! info.Length()) {
		return Nan::ThrowTypeError("Too few arguments.");
	}

	if (! info[0]->IsObject()) {
		return Nan::ThrowTypeError("First argument MUST be an object.");
	}

	options = Local<Object>::Cast(info[0]);

	// Unwrap C++ object and apply its new options
	engine = AudioEngine::Unwrap<AudioEngine>(info.This());
	engine->applyOptions(options);

	info.GetReturnValue().SetUndefined();
}

/**
 * Apply options to the native C++ object and restart the stream with given
 * options. Furthermore, it will also reset (free + allocate) temporary buffers
 * with new parameters.
 */
void Audio::AudioEngine::applyOptions(Local<Object> options) {
	unsigned int prevNumBuffers = numBuffers;
	int formatID = 0;

	VALIDATE_PROPERTY(options, "inputDevice",     int,  inputDevice);
	VALIDATE_PROPERTY(options, "outputDevice",    int,  outputDevice);
	VALIDATE_PROPERTY(options, "inputChannels",   int,  inputChannels);
	VALIDATE_PROPERTY(options, "outputChannels",  int,  outputChannels);
	VALIDATE_PROPERTY(options, "framesPerBuffer", int,  samplesPerBuffer);
	VALIDATE_PROPERTY(options, "numBuffers",      int,  numBuffers);
	VALIDATE_PROPERTY(options, "interleaved",     bool, interleaved);
	VALIDATE_PROPERTY(options, "useMicrophone",   bool, readMicrophone);
	VALIDATE_PROPERTY(options, "sampleRate",      int,  sampleRate);
	VALIDATE_PROPERTY(options, "sampleFormat",    int,  formatID);

	switch (formatID) {
		case 0x01:
			sampleFormat = paFloat32;
			sampleSize   = 4;
			break;
		case 0x02:
			sampleFormat = paInt32;
			sampleSize   = 4;
			break;
		case 0x04:
			sampleFormat = paInt24;
			sampleSize   = 3;
			break;
		case 0x08:
			sampleFormat = paInt16;
			sampleSize   = 2;
			break;
		case 0x10:
			sampleFormat = paInt8;
			sampleSize   = 1;
			break;
		case 0x20:
			sampleFormat = paUInt8;
			sampleSize   = 1;
			break;
	}

	// Setup our input and output parameters
	streamInputParams.device = inputDevice;
	streamInputParams.channelCount = inputChannels;
	streamInputParams.sampleFormat = sampleFormat;
	streamInputParams.suggestedLatency = Pa_GetDeviceInfo(inputDevice)->defaultLowInputLatency;
	streamInputParams.hostApiSpecificStreamInfo = NULL;

	streamOutputParams.device = outputDevice;
	streamOutputParams.channelCount = outputChannels;
	streamOutputParams.sampleFormat = sampleFormat;
	streamOutputParams.suggestedLatency = Pa_GetDeviceInfo(outputDevice)->defaultLowOutputLatency;
	streamOutputParams.hostApiSpecificStreamInfo = NULL;

	// Clear out our temp buffer blocks
	if(cachedInputSampleBlock != NULL) {
		free(cachedInputSampleBlock);
	}

	if(cachedOutputSampleBlock != NULL) {
		for (unsigned int i = 0; i < prevNumBuffers; i++) {
			if (cachedOutputSampleBlock[i] != NULL) {
				free(cachedOutputSampleBlock[i]);
			}
		}
		free(cachedOutputSampleBlock);
	}

	if (cachedOutputSampleBlockForWriting != NULL) {
		free(cachedOutputSampleBlockForWriting);
	}

	// Allocate some new space for our temp buffer blocks
	cachedInputSampleBlock  = (char*)malloc(samplesPerBuffer * inputChannels * sampleSize);
	cachedOutputSampleBlock = (char**)malloc(sizeof(char*) * numBuffers);
	for (int i = 0; i < numBuffers; i++) {
		cachedOutputSampleBlock[i] = (char*)malloc(samplesPerBuffer * outputChannels * sampleSize);
	}
	cachedOutputSampleBlockForWriting = (char*)malloc(samplesPerBuffer * outputChannels * sampleSize);
	numCachedOutputSamples = (unsigned int*)calloc(sizeof(unsigned int), numBuffers);

	// Reinitialize audio stream with new options.
	if(stream != NULL && Pa_IsStreamActive(stream)) {
		reinitializeStream();
	}
}

void Audio::AudioEngine::printOptions() {
	fprintf(stderr, "input :%d\n",          inputDevice);
	fprintf(stderr, "output :%d\n",         outputDevice);
	fprintf(stderr, "rate :%d\n",           sampleRate);
	fprintf(stderr, "format :%d\n",         sampleFormat);
	fprintf(stderr, "size :%ld\n",          sizeof(float));
	fprintf(stderr, "inputChannels :%d\n",  inputChannels);
	fprintf(stderr, "outputChannels :%d\n", outputChannels);
	fprintf(stderr, "interleaved :%d\n",    interleaved);
	fprintf(stderr, "uses input: %d\n",     readMicrophone);
}

/**
 * Checks if the current audio buffer has been fed to Port Audio.
 */
void Audio::AudioEngine::isBufferEmpty(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	AudioEngine *engine;
	Local<Boolean> isEmpty;

	engine = AudioEngine::Unwrap<AudioEngine>(info.This());

	uv_mutex_lock(&engine->m_mutex);
	isEmpty = Nan::New<Boolean>(engine->numCachedOutputSamples[engine->currentWriteBuffer] == 0);
	uv_mutex_unlock(&engine->m_mutex);

	info.GetReturnValue().Set(isEmpty);
}


/**
 * Write samples to the current audio device.
 */
void Audio::AudioEngine::write(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	AudioEngine *engine;

	engine = AudioEngine::Unwrap<AudioEngine>(info.This());

	if (info.Length() > 1 || ! info[0]->IsArray()){
		return Nan::ThrowTypeError("First argument should be an array.");
	}

	uv_mutex_lock(&engine->m_mutex);
	engine->queueOutputBuffer(Local<Array>::Cast(info[0]));
	uv_mutex_unlock(&engine->m_mutex);

	// NOTE: Why returning false ?
	info.GetReturnValue().Set(false);
}

/**
 * Read samples from the current audio device.
 */
void Audio::AudioEngine::read(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	AudioEngine *engine;
	Local<Array> input;

	engine = AudioEngine::Unwrap<AudioEngine>(info.This());

	if (engine->readMicrophone) {
		Pa_ReadStream(engine->stream, engine->cachedInputSampleBlock, engine->samplesPerBuffer);
	}

	input = engine->getInputBuffer();

	info.GetReturnValue().Set(input);
}


/**
 * Returns whether the PortAudio stream is active.
 */
void Audio::AudioEngine::isActive(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	AudioEngine* engine;

	// Unwrap and return status from PortAudio engine
	engine = AudioEngine::Unwrap<AudioEngine>(info.This());
	info.GetReturnValue().Set( Nan::New<Boolean>(Pa_IsStreamActive(engine->stream)));
}

/**
 * Sets a sample in the queued output buffer.
 * @TODO: ASAP
 * @NOTE: VERY NOT optimized, we could define an handler (callback function)
 * depending sample format and then call it from #queueOutputBuffer. It could
 * replace the switch at each #setSample.
 */
void Audio::AudioEngine::setSample(int position, Handle<Value> sample) {
	int temp;
	switch(sampleFormat) {
		case paFloat32:
			((float *)cachedOutputSampleBlock[currentWriteBuffer])[position] = (float)sample->NumberValue();
			break;

		case paInt32:
			((int *)cachedOutputSampleBlock[currentWriteBuffer])[position] = (int)sample->NumberValue();
			break;

		case paInt24:
			temp = (int)sample->NumberValue();
			cachedOutputSampleBlock[currentWriteBuffer][3*position + 0] = temp >> 16;
			cachedOutputSampleBlock[currentWriteBuffer][3*position + 1] = temp >> 8;
			cachedOutputSampleBlock[currentWriteBuffer][3*position + 2] = temp;
			break;

		case paInt16:
			((int16_t *)cachedOutputSampleBlock[currentWriteBuffer])[position] = (int16_t)sample->NumberValue();
			break;

		default:
			cachedOutputSampleBlock[currentWriteBuffer][position] = (char)sample->NumberValue();
			break;
	}
}

/**
 * Returns a sound card sample converted to a v8 Number.
 * @TODO: ASAP
 * @NOTE: VERY NOT optimized, we could define an handler (callback function)
 * depending sample format and then call it from #getInputBuffer. It could
 * replace the switch at each #getSample.
 */

Handle<Number> Audio::AudioEngine::getSample(int position) {
	Nan::EscapableHandleScope scope;
	float fValue;
	Local<Number> sample;

	switch(sampleFormat ) {
		case paFloat32:
			fValue = ((float*)cachedInputSampleBlock)[position];
			sample = Nan::New<Number>( fValue );
			break;

		case paInt32:
			sample = Nan::New<Integer>(((int*)cachedInputSampleBlock)[position]);
			break;

		case paInt24:
			sample = Nan::New<Integer>(
					(cachedInputSampleBlock[3*position + 0] << 16)
				  + (cachedInputSampleBlock[3*position + 1] << 8)
				  + (cachedInputSampleBlock[3*position + 2]));
			break;

		case paInt16:
			sample = Nan::New<Integer>(((int16_t*)cachedInputSampleBlock)[position]);
			break;

		default:
			sample = Nan::New<Integer>(cachedInputSampleBlock[position]* - 1);
			break;
	}

	return scope.Escape(sample);
}

/**
 * Returns a v8 array filled with input samples.
 */
Local<Array> Audio::AudioEngine::getInputBuffer() {
	Nan::EscapableHandleScope scope;

	if (interleaved) {
		int sample;
		int samples = inputChannels * samplesPerBuffer;

		inputBuffer = Nan::New<Array>(samples);
		for(sample = 0; sample < samples; sample++) {
			inputBuffer->Set(sample, getSample(sample));
		}
	} else {
		int channel, sample;

		inputBuffer = Nan::New<Array>(inputChannels);
		for(channel = 0; channel< inputChannels; channel++ ) {
			auto tmpBuffer = Local<Array>(Nan::New<Array>(samplesPerBuffer));

			for (sample = 0; sample < samplesPerBuffer; sample++) {
				tmpBuffer->Set(sample, getSample(sample));
			}

			inputBuffer->Set(channel, tmpBuffer);
		}
	}

	return scope.Escape(inputBuffer);
}

/**
 * Queues up an array to be sent to the sound card.
 */
void Audio::AudioEngine::queueOutputBuffer(Handle<Array> result) {
	// Reset our record of the number of cached output samples
	numCachedOutputSamples[currentWriteBuffer] = 0;

	if (interleaved) {
		int sample;
		for(sample = 0; sample < samplesPerBuffer * outputChannels; sample++) {
			setSample( sample, result->Get(sample) );
		}

		numCachedOutputSamples[currentWriteBuffer] = result->Length() / outputChannels;
	} else {
		Local<Array> item;
		int channel, sample;

		for(channel = 0; channel < outputChannels; channel++) {
			for(sample = 0; sample < samplesPerBuffer; sample++) {
				item = Local<Array>::Cast(result->Get(channel));
				if (! item->IsArray()) {
					Nan::ThrowTypeError("Output buffer not properly setup, channel is not an array");
					return;
				}

				if (item->Length() > numCachedOutputSamples[currentWriteBuffer]) {
					numCachedOutputSamples[currentWriteBuffer] = item->Length();
				}

				setSample(sample, item->Get(sample));
			}
		}
	}
	currentWriteBuffer = (currentWriteBuffer + 1) % numBuffers;
}


/**
 * Run the main blocking audio loop.
 * @TODO: let's have a look if we cannot do without a loop...
 */
void Audio::AudioEngine::RunAudioLoop(){
	PaError error = 0;
	int numSamples;

	assert(stream);

	// NOTE: while(true) is never a good idea...
	while (true) {
		inputOverflowed = (error != paNoError);
		numSamples = 0;

		uv_mutex_lock(&m_mutex);
		// TODO: Don't want to read that today ... to review..
		if (numCachedOutputSamples[currentReadBuffer] > 0) {
			memcpy(cachedOutputSampleBlockForWriting, cachedOutputSampleBlock[currentReadBuffer], numCachedOutputSamples[currentReadBuffer] * outputChannels * sampleSize);
			numSamples = numCachedOutputSamples[currentReadBuffer];
			numCachedOutputSamples[currentReadBuffer] = 0;
			currentReadBuffer = (currentReadBuffer + 1) % numBuffers;
		}
		uv_mutex_unlock(&m_mutex);

		if(numSamples > 0) {
			error = Pa_WriteStream(stream, cachedOutputSampleBlockForWriting, numSamples);
			outputUnderflowed = (error != paNoError);
		} else {
			outputUnderflowed = true;
#if defined( __WINDOWS__ ) || defined( _WIN32 )
			Sleep(1);
#else
			sleep(1);
#endif
		}
	}
}

/**
 * Get the name of an audio device with a given ID number.
 */
void Audio::AudioEngine::getDeviceName(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Local<Number> index;
	const PaDeviceInfo *device;

	if (! info[0]->IsNumber()) {
		return Nan::ThrowTypeError("To few arguments.");
	}

	index  = Local<Number>::Cast(info[0]);
	device = Pa_GetDeviceInfo((PaDeviceIndex) index->NumberValue());

	info.GetReturnValue().Set(Nan::New<String>(device->name).ToLocalChecked());
}


/**
 * Get the number of available devices.
 * @static
 */
void Audio::AudioEngine::getNumDevices(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;

	info.GetReturnValue().Set(Nan::New<Number>(Pa_GetDeviceCount()));
}


/**
 * Wraps a handle into an object.
 * NOTE: Is it mandatory ?? Do we call this ? If yes, when and why ?
 */
void Audio::AudioEngine::wrapObject(v8::Handle<v8::Object> object) {
	ObjectWrap::Wrap(object);
}


/**
 * Native C++ AudioEngine destructor.
 */
Audio::AudioEngine::~AudioEngine() {
	// TODO Nothing todo here since we initialize PortAudio in library
	// constructor.
	// However I still have to write library destructor with Pa_Terminate...
	//
	// if (Pa_Terminate() != paNoError)
	//	fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
	//
}
