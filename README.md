This module is in transition. The master branch should work for mac and linux computers, and the v0.1.0_windows branch should work for windows. 

Node Core Audio
==================
This is a C++ extension for node.js that gives javascript access to audio buffers and basic audio processing functionality

Right now, it's basically a node.js binding for PortAudio.

Installation
=====

On OSX/Linux you need portaudio libs installed.
```
	sudo apt-get intall portaudio
	sudo port install portaudio
	etc.
```

```
npm install node-core-audio
```

Disclaimer
=====
This extension is still under heavy development! Please contact me with any questions
or issues.

I am actively working on this, but if you want to see it happen faster, please 
send me an email!

Basic Usage
=====
The basis usage is as a "pull" system, meaning that the audio engine will 
call your processing function when it's ready to get new samples. "push" methods
where you post audio to the sound card are available as well.

Below is the most basic use of the audio engine. We create a new instance of
node-core-audio, and then give it our processing function. The audio engine
will call the audio callback whenever it needs an output buffer to send to
the sound card.

```javascript
// Create a new instance of node-core-audio
var coreAudio = require("node-core-audio").createNewAudioEngine();

// Add an audio processing callback
// Note: This function should return a buffer to the audio engine!
// It is used for the output.
var output = [];
coreAudio.createAudioEngine(
	{
		inputChannels: 1,
		outputChannels: 2
	},
	function(input, lastInputOverflowed, lastOutputUnderflowed) {
		output[0] = output[1] = input[0]; //copy mono input to stereo
        return output; //returning an buffer means, writing to the soundcard
    }
);

//Alternatively, you can write/read manually.
var audio = coreAudio.createAudioEngine({outputChannels: 2});
var output = [
	[<some samples>], //left
	[<some samples>]  //right
]
audio.write(output);
var input = audio.read();
output[0] = output[1] = input[0]; //copy mono input to stereo
audio.write(output);
```

Important! Processing Thread
=====
When you are writing code inside of your audio callback, you are operating on
the processing thread of the application. The callback runs at very high priority,
depending on framesPerBuffer (default is 256, the lower the lower is the delay between
each call). Expensive operations inside this function will cause a lack in the output.
You see with lastInputOverflowed and lastOutputUnderflowed whether your function is
too slow or not. (value of 0 means everything is fine)

The basic principle is that you should have everything ready to go before you enter
the processing function. Buffers, objects, and functions should be created in a 
constructor or static function outside of the audio callback.

The callback is only called if all buffers has been processed by the soundcard.

Audio Engine Options
=====
* sampleRate [default 44100]: Sample rate - number of samples per second in the audio stream
* sampleFormat [default sampleFormatFloat32]: Bit depth - Number of bits used to represent sample values
  formats are sampleFormatFloat32, sampleFormatInt32, sampleFormatInt24, sampleFormatInt16, sampleFormatInt8, sampleFormatUInt8.
* framesPerBuffer [default 256]: Buffer length - Number of samples per buffer
* interleaved [default false]: Interleaved / Deinterleaved - determines whether samples are given to you as a two dimensional array (buffer[channel][sample]) (deinterleaved) or one buffer with samples from alternating channels (interleaved).
* inputChannels [default 2]: Input channels - number of input channels
* outputChannels [default 2]: Output channels - number of output channels
* inputDevice [default to Pa_GetDefaultInputDevice]: Input device - id of the input device
* outputDevice [default to Pa_GetDefaultOutputDevice]: Output device - id of the output device

API
=====
```javascript
var coreAudio = require("node-core-audio");

// Initialize the audio engine
var output = [];
var audio = coreAudio.createAudioEngine(
	{
		inputChannels: 1,
		outputChannels: 6
	},
	function(input, lastInputOverflowed, lastOutputUnderflowed) {
		output[0] = output[1] = output[2] = output[3] = output[4] = output[5] = input[0];
        return output; //just copy input (mono) to each channel of our 5.1 output
    }
);

// Returns whether the audio engine is active
bool audio.isActive();

// Updates the parameters and restarts the engine. All keys from getOptions() are available.
audio.setOptions({
	inputChannels: 2
});

//Returns all parameters
array audio.getOptions();
=> {
	inputChannels: int,
	outputChannels: int,
	inputDevice: int,
	outputDevice: int,
	sampleRate: int,
	sampleFormat: int,
	framesPerBuffer: int,
	interleaved: bool
};

// Reads buffer of the input of the soundcard and returns as array.
// Interlaced oder Deinterlaced, depends on your options. Default is deinterlaced array (buffer[channel][sample])
// notic: blocking i/o
array audio.read();

// Writes the buffer to the output of the soundcard. Returns false if underflowed.
// notic: blocking i/o
bool audio.write(array input);

// Returns the name of a given device 
string audio.getDeviceName( int inputDeviceIndex );

// Returns the total number of audio devices
int audio.getNumDevices();

```

Known Issues / TODO
=====

* Add FFTW to C++ extension, so you can get fast FFT's from javascript, and also register for the FFT of incoming audio, rather than the audio itself
* Add support for streaming audio over sockets

License
=====
MIT - See LICENSE file.
