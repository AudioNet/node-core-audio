Node Core Audio
==================

![alt tag](https://nodei.co/npm-dl/node-core-audio.png)

A C++ extension for node.js that gives javascript access to audio buffers and basic audio processing functionality

Right now, it's basically a node.js binding for PortAudio.

NOTE: Looking for help maintaining this repository!

Active contributors:

- [rmedaer](https://github.com/rmedaer)

Installation
=====

```
npm install node-core-audio
```

Basic Usage
=====

Below is the most basic use of the audio engine. We create a new instance of
node-core-audio, and then give it our processing function. The audio engine
will call the audio callback whenever it needs an output buffer to send to
the sound card.

```javascript
// Create a new instance of node-core-audio
var coreAudio = require("node-core-audio");

// Create a new audio engine
var engine = coreAudio.createNewAudioEngine();

// Add an audio processing callback
// This function accepts an input buffer coming from the sound card,
// and returns an ourput buffer to be sent to your speakers.
//
// Note: This function must return an output buffer
function processAudio( inputBuffer ) {
	console.log( "%d channels", inputBuffer.length );
	console.log( "Channel 0 has %d samples", inputBuffer[0].length );

	return inputBuffer;
}

engine.addAudioCallback( processAudio );
```

// Alternatively, you can read/write samples to the sound card manually
```javascript
var engine = coreAudio.createNewAudioEngine();

// Grab a buffer
var buffer = engine.read();

// Silence the 0th channel
for( var iSample=0; iSample<inputBuffer[0].length; ++iSample )
	buffer[0][iSample] = 0.0;

// Send the buffer back to the sound card
engine.write( buffer );
```

Important! Processing Thread
=====
When you are writing code inside of your audio callback, you are operating on
the processing thread of the application. This high priority environment means you
should try to think about performance as much as possible. Allocations and other
complex operations are possible, but dangerous.

IF YOU TAKE TOO LONG TO RETURN A BUFFER TO THE SOUND CARD, YOU WILL HAVE AUDIO DROPOUTS

The basic principle is that you should have everything ready to go before you enter
the processing function. Buffers, objects, and functions should be created in a constructor or static function outside of the audio callback whenever possible. The
examples in this readme are not necessarily good practice as far as performance is concerned.

The callback is only called if all buffers has been processed by the soundcard.

Audio Engine Options
=====
* sampleRate [default 44100]
  * Sample rate - number of samples per second in the audio stream
* sampleFormat [default sampleFormatFloat32]
  * Bit depth - Number of bits used to represent sample values
  * formats are sampleFormatFloat32, sampleFormatInt32, sampleFormatInt24, sampleFormatInt16, sampleFormatInt8, sampleFormatUInt8.
* framesPerBuffer [default 256]
  * Buffer length - Number of samples per buffer
* interleaved [default false]
  * Interleaved / Deinterleaved - determines whether samples are given to you as a two dimensional array (buffer[channel][sample]) (deinterleaved) or one buffer with samples from alternating channels (interleaved).
* inputChannels [default 2]
  * Input channels - number of input channels
* outputChannels [default 2]
  * Output channels - number of output channels
* inputDevice [default to Pa_GetDefaultInputDevice]
  * Input device - id of the input device
* outputDevice [default to Pa_GetDefaultOutputDevice]
  * Output device - id of the output device

API
=====
First things first
```javascript
var coreAudio = require("node-core-audio");
```
Create and audio processing function
```javascript
function processAudio( inputBuffer ) {
    // Just print the value of the first sample on the left channel
    console.log( inputBuffer[0][0] );
}
```

Initialize the audio engine and setup the processing loop
```javascript
var engine = coreAudio.createNewAudioEngine();

engine.addAudioCallback( processAudio );
```

General functionality
```javascript
// Returns whether the audio engine is active
bool engine.isActive();

// Updates the parameters and restarts the engine. All keys from getOptions() are available.
engine.setOptions({
	inputChannels: 2
});

// Returns all parameters
array engine.getOptions();

// Reads buffer of the input of the soundcard and returns as array.
// Note: this is a blocking call, don't take too long!
array engine.read();

// Writes the buffer to the output of the soundcard. Returns false if underflowed.
// notic: blocking i/o
bool engine.write(array input);

// Returns the name of a given device
string engine.getDeviceName( int inputDeviceIndex );

// Returns the total number of audio devices
int engine.getNumDevices();
```

Known Issues / TODO
=====

* Add FFTW to C++ extension, so you can get fast FFT's from javascript, and also register for the FFT of incoming audio, rather than the audio itself
* Add support for streaming audio over sockets


License
=====
MIT - See LICENSE file.

Copyright Mike Vegeto, 2013
