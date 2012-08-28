Node Core Audio
==================
This is a C++ extension for node.js that gives javascript access to audio buffers and basic audio processing functionality

Right now, it's basically a node.js binding for PortAudio.

Installation
=====
```
npm install node-core-audio
```

Disclaimer
=====
This extension is still under heavy development, and should be considered 
pre-alpha. I wouldn't suggest you even try to use it, unless you're willing to roll up your
sleeves from time to time.

I am actively working on this, but if you want to see it happen faster, please 
send me an email!

Basic Usage
=====
This package is setup as a "pull" system, meaning that the audio engine will 
call your processing function when it's ready to get new samples, as opposed
to a "push" system, where you post audio to the sound card.

Below is the most basic use of the audio engine. We create a new instance of
node-core-audio, and then give it our processing function. The audio engine
will call the audio callback whenever it needs an output buffer to send to
the sound card.
```javascript
// Create a new instance of node-core-audio
var coreAudio = require("node-core-audio").createNewAudioEngine();

// Add an audio processing callback
// Note: This function MUST return a buffer to the audio engine! If 
// not, your application will throw an exception.
coreAudio.addAudioCallback( function( numSamples, inputBuffer ) {
	console.log( "sweet" );
	return inputBuffer;
});
```

Important! Processing Thread
=====
When you are writing code inside of your audio callback, you are operating on
the processing thread of the application. Any large allocations will cause the
program to crash, and you will get no error whatsoever. This is a bug, and I
hope to fix it soon.

DO NOT DO THESE INSIDE THE AUDIO CALLBACK (or anything like them):
```javascript
var array = [];
var array = new Array();		// Allocating for arrays is not okay
var object = {};				// Allocating for objects is not okay
var callback = function(){};	// Allocating for functions is not okay
socket.emit(...);				// Doing socket stuff is not okay (because it causes object/array allocations)
```

The basic principle is that you should have everything ready to go before you enter
the processing function. Buffers, objects, and functions should be created in a 
constructor or static function outside of the audio callback.

Simple allocations and assignments are okay (you CAN do these):
```javascript
// Simple allocation
var someValue = 10;	

// Assignment of whole buffers
preAllocatedBuffer = inputBuffer;

// Sample by sample processing
for( var iSample = 0; iSample < numSamples; ++iSample ) {
	preAllocatedBuffer[iSample] = numSamples / iSample;
}
```

Audio Engine Options (not implimented as of version 0.0.5)
=====
* Sample rate - number of samples per second in the audio stream
* Bit depth - Number of bits used to represent sample values
* Buffer length - Number of samples per buffer
* Interlaced / Deinterlaced - determines whether samples are given to you as a two dimensional array (buffer[channel][sample]) or one buffer with samples from alternating channels

API (much more to come!)
=====
```javascript
var coreAudio = require("node-core-audio").createNewAudioEngine();

// Adds an audio callback to the audio engine (MUST RETURN AN OUTPUT BUFFER)
coreAudio.addAudioCallback( function(numSamples, inputBuffer){ return inputBuffer; } );

// Returns whether the audio engine is active
var isActive = coreAudio.isActive();

// Returns the sample rate of the audio engine
var sampleRate = coreAudio.getSampleRate();

// Returns the index of the input audio device 
var inputDeviceIndex = coreAudio.getInputDeviceIndex();

// Returns the index of the output audio device 
var outputDeviceIndex = coreAudio.getOutputDeviceIndex();

// Returns the name of a given device 
var inputDeviceName = coreAudio.getDeviceName( inputDeviceIndex );

// Returns the total number of audio devices
var numDevices = coreAudio.getNumDevices();

// Returns the number of input channels
var numChannels = coreAudio.getNumInputChannels();

// Returns the number of output channels
var numChannels = coreAudio.getNumOutputChannels();

// Sets the input audio device
coreAudio.setInputDevice( someDeviceId );

// Sets the output audio device
coreAudio.setOutputDevice( someDeviceId );
```

Known Issues / TODO
=====

* Create thread for Javascript/UI - allocations from javascript shouldn't cause crashes
* Add FFTW to C++ extension, so you can get fast FFT's from javascript, and also register for the FFT of incoming audio, rather than the audio itself
* Add support for streaming audio over sockets

License - MIT
=====
Copyright (c) 2012, Mike Vegeto (michael.vegeto@gmail.com)
All rights reserved.

See LICENSE file