//////////////////////////////////////////////////////////////////////////
// node-core-audio - main module
//////////////////////////////////////////////////////////////////////////
//
// Main javascript audio API
/* ----------------------------------------------------------------------
													Object Structures
-------------------------------------------------------------------------
	
*/
//////////////////////////////////////////////////////////////////////////
// Node.js Exports
var globalNamespace = {};
(function (exports) {
	exports.createNewAudioEngine = function( options ) {
		newAudioEngine= new AudioEngine( options );
		return newAudioEngine;
	};
}(typeof exports === 'object' && exports || globalNamespace));

var FFT = require("fft");


//////////////////////////////////////////////////////////////////////////
// Namespace (lol)
var SHOW_DEBUG_PRINTS = true;
var MAX_SUPPORTED_CHANNELS = 6;													// We need to allocate our process audio for the max channels, 
																				// so we have to set some reasonable limit																				
var log = function( a ) { if(SHOW_DEBUG_PRINTS) console.log(a); };				// A log function we can turn off
var exists = function(a) { return typeof(a) == "undefined" ? false : true; };	// Check whether a variable exists


//////////////////////////////////////////////////////////////////////////
// Constructor
function AudioEngine( options ) {
	var audioEngineImpl = require( __dirname + "/build/Debug/NodeCoreAudio" );

	var defaultOptions = {
		inputChannels: 1,
		outputChannels: 2
	};

	var callback = function() {
        return input;
    }

    this.options = options || defaultOptions;
	
	this.audioEngine = audioEngineImpl.createAudioEngine( this.options, callback );
	this.audioStreamer;
	
	this.processingCallbacks = [];
	this.uiUpdateCallbacks = [];
	this.didProcessAudio = false;
	
	this.outputBuffer = [];
	this.tempBuffer = [];
	this.processBuffer = [];

	this.fft = new FFT.complex( this.audioEngine.getOptions().framesPerBuffer, false );
	this.fftBuffer = [];
	
	var _this = this;

	function validateOutputBufferStructure( buffer ) {
		if( buffer === undefined ) {
			console.log( "Audio processing function didn't return an output buffer" );
			return false;
		}
		
		if( !_this.audioEngine.getOptions().interleaved ) {

			if( buffer.length > _this.options.inputChannels ) {
				console.log( "Output buffer has info for too many channels" );
				return false;
			} else if( buffer.length < _this.options.inputChannels ) {
				console.log( "Output buffer doesn't have data for enough channels" );
				return false;
			}

			if( typeof(buffer[0]) != "object" ) { 
				console.log( "Output buffer not setup correctly, buffer[0] isn't an array" );
				return false;
			}

			if( typeof(buffer[0][0]) != "number" ) {
				console.log( "Output buffer not setup correctly, buffer[0][0] isn't a number" );
				return false;
			}
		} else {
			if( typeof(buffer[0]) != "number" ) {
				console.log( "Output buffer not setup correctly, buffer[0] isn't a number" );
				return false;
			}
		}

		return true;
	}

	// Allocate a processing buffer for each of our channels
	for( var iChannel = 0; iChannel<MAX_SUPPORTED_CHANNELS; ++iChannel ) {
		this.processBuffer[iChannel] = [];
	}
	
	// Start polling the audio engine for data as fast as we can	
	var _this = this;
	setInterval( function() {	
		_this.didProcessAudio = false;
		
		// Try to process audio
		var input = _this.audioEngine.read();

		var outputBuffer = _this.getProcessAudio()( input );

		if( validateOutputBufferStructure(outputBuffer) )
			_this.audioEngine.write( outputBuffer );
		
		// Call our UI updates now that all the DSP work has been done
		for( var iUpdate=0; iUpdate < _this.uiUpdateCallbacks.length; ++iUpdate ) {
			_this.uiUpdateCallbacks[iUpdate]();
		}
	}, 0 );
} // end AudioEngine();


//////////////////////////////////////////////////////////////////////////
// Returns our main audio processing function
AudioEngine.prototype.getProcessAudio = function() {
	var self = this;

	var options = this.audioEngine.getOptions(),
		numChannels = options.inputChannels,
		fftBuffer = this.fftBuffer;
	
	var processAudio = function( inputBuffer ) {	

		// If we don't have any processing callbacks, just get out
		if( self.processingCallbacks.length == 0 )
			return inputBuffer;
			
		var processBuffer = self.processBuffer;
			
		if( !self.options.interleaved )
			deInterleave( inputBuffer, processBuffer, processBuffer[0].length, numChannels );

		// Call through to all of our processing callbacks
		for( var iCallback = 0; iCallback < self.processingCallbacks.length; ++iCallback ) {
			processBuffer = self.processingCallbacks[iCallback]( processBuffer );
		} // end for each callback
		
		
		if( typeof(self.audioStreamer) != "undefined" ) {
			self.audioStreamer.streamAudio( processBuffer, processBuffer[0].length, numChannels );
		}
		
		var outputBuffer = self.outputBuffer;
		
		if( !self.options.interleaved )
			interleave( processBuffer, outputBuffer, processBuffer[0].length, numChannels );
		
		self.didProcessAudio = true;
		
		// Return our output audio to the sound card
		return outputBuffer;
	} // end processAudio()
	
	return processAudio;
} // end AudioEngine.getProcessAudio()


//////////////////////////////////////////////////////////////////////////
// Get the engine's options 
AudioEngine.prototype.getOptions = function() {
	return this.audioEngine.getOptions();
} // end AudioEngine.getOptions()


//////////////////////////////////////////////////////////////////////////
// Get the engine's options 
AudioEngine.prototype.setOptions = function( options ) {
	this.options = options;
	this.audioEngine.setOptions( this.options );
} // end AudioEngine.setOptions()


//////////////////////////////////////////////////////////////////////////
// Add a processing callback 
AudioEngine.prototype.createAudioHub = function( port ) {
	this.audioStreamer = require("audio-streamer").createNewAudioStreamer( port );
} // end AudioEngine.createAudiohub()


//////////////////////////////////////////////////////////////////////////
// Add a processing callback 
AudioEngine.prototype.addAudioCallback = function( callback ) {
	this.processingCallbacks.push( callback );
} // end AudioEngine.addAudioCallback()


//////////////////////////////////////////////////////////////////////////
// Add a UI update callback
AudioEngine.prototype.addUpdateCallback = function( callback ) {
	this.uiUpdateCallbacks.push( callback );
} // end AudioEngine.addUpdateCallback()


//////////////////////////////////////////////////////////////////////////
// Returns whether the audio engine is active 
AudioEngine.prototype.isActive = function() {
	return this.audioEngine.isActive();
} // end AudioEngine.isActive()


//////////////////////////////////////////////////////////////////////////
// Returns the sample rate of the audio engine 
AudioEngine.prototype.getSampleRate = function() {
	return this.audioEngine.getSampleRate();
} // end AudioEngine.getSampleRate()


//////////////////////////////////////////////////////////////////////////
// Returns the index of the input audio device 
AudioEngine.prototype.getInputDeviceIndex = function() {
	return this.audioEngine.getInputDeviceIndex();
} // end AudioEngine.getInputDeviceIndex()


//////////////////////////////////////////////////////////////////////////
// Returns the index of the output audio device 
AudioEngine.prototype.getOutputDeviceIndex = function() {
	return this.audioEngine.getOutputDeviceIndex();
} // end AudioEngine.getOutputDeviceIndex()


//////////////////////////////////////////////////////////////////////////
// Returns the name of a given device 
AudioEngine.prototype.getDeviceName = function( deviceId ) {
	return this.audioEngine.getDeviceName( deviceId );
} // end AudioEngine.getDeviceName()


//////////////////////////////////////////////////////////////////////////
// Returns the total number of audio devices
AudioEngine.prototype.getNumDevices = function() {
	return this.audioEngine.getNumDevices();
} // end AudioEngine.getNumDevices()


//////////////////////////////////////////////////////////////////////////
// Sets the input audio device
AudioEngine.prototype.setInputDevice = function( deviceId ) {
	return this.audioEngine.setInputDevice( deviceId );
} // end AudioEngine.setInputDevice()


//////////////////////////////////////////////////////////////////////////
// Sets the output audio device
AudioEngine.prototype.setOutputDevice = function( deviceId ) {
	return this.audioEngine.setOutputDevice( deviceId );
} // end AudioEngine.setOutputDevice()


//////////////////////////////////////////////////////////////////////////
// Returns the number of input channels
AudioEngine.prototype.getNumInputChannels = function() {
	return this.audioEngine.getNumInputChannels();
} // end AudioEngine.getNumInputChannels()


//////////////////////////////////////////////////////////////////////////
// Returns the number of output channels
AudioEngine.prototype.getNumOutputChannels = function() {
	return this.audioEngine.getNumOutputChannels();
} // end AudioEngine.getNumOutputChannels()


//////////////////////////////////////////////////////////////////////////
// Read audio samples from the sound card 
AudioEngine.prototype.read = function() {
	return this.audioEngine.read();
} // end AudioEngine.read()


//////////////////////////////////////////////////////////////////////////
// Write some audio samples to the sound card
AudioEngine.prototype.write = function() {
	this.audioEngine.write();
} // end AudioEngine.write()


//////////////////////////////////////////////////////////////////////////
// Splits a 1d buffer into its channel components
function deInterleave( inputBuffer, outputBuffer, numSamplesPerBuffer, numChannels ) {
	if( numChannels < 2 ) {
		outputBuffer[0] = inputBuffer;
		return;
	}

	for( var iSample = 0; iSample < numSamplesPerBuffer; iSample += numChannels ) {
		for( var iChannel = 0; iChannel < numChannels; ++iChannel ) {
			outputBuffer[iChannel][iSample] = inputBuffer[iSample + iChannel];
		} // end for each channel		
	} // end for each sample position
} // end deInterleave()


//////////////////////////////////////////////////////////////////////////
// Joins multidimensional array into single buffer
function interleave( inputBuffer, outputBuffer, numSamplesPerBuffer, numChannels ) {
	if( numChannels < 2 ) {
		outputBuffer[0] = inputBuffer;
		return;
	}

	for( var iChannel = 0; iChannel < numChannels; ++iChannel ) {
		if( inputBuffer[iChannel] === undefined ) break;

		for( var iSample = 0; iSample < numSamplesPerBuffer; iSample += numChannels ) {
			outputBuffer[iSample + iChannel] = inputBuffer[iChannel][iSample];
		} // end for each sample position		
	} // end for each channel	

} // end interleave()