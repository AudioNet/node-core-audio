Node Core Audio
==================
This is a C++ extension for node.js that gives javascript access to audio buffers and basic audio processing functionality

Right now, it's basically a node.js binding for PortAudio.

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

Audio Engine Options (these are not fully implimented as of version 0.0.4)
=====
* Sample rate - number of samples per second in the audio stream
* Bit depth - Number of bits used to represent sample values
* Buffer length - Number of samples per buffer
* Interlaced / Deinterlaced - determines whether samples are given to you as a two dimensional array (buffer[channel][sample]) or one buffer with samples from alternating channels

API (much more to come!)
=====
```javascript
coreAudio.addAudioCallback( ...your processing function... );
```

Known Issues / TODO
=====
I know, I know, I'm sorry. I'll fix them, don't worry.

* Create thread for Javascript/UI - allocations from javascript shouldn't cause crashes
* Add FFTW to C++ extension, so you can get fast FFT's from javascript, and also register for the FFT of incoming audio, rather than the audio itself
* Add support for streaming audio over sockets

License
=====
Copyright (c) 2012, Mike Vegeto (michael.vegeto@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MIKE VEGETO BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.