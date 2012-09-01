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
	exports.createNewAudioEngine = function() {
		newAudioEngine= new AudioEngine();
		return newAudioEngine;
	};
}(typeof exports === 'object' && exports || globalNamespace));


//////////////////////////////////////////////////////////////////////////
// Namespace (lol)
var SHOW_DEBUG_PRINTS = true;
var MAX_SUPPORTED_CHANNELS = 6;													// We need to allocate our process audio for the max channels, 
																				// so we have to set some reasonable limit																				
var log = function( a ) { if(SHOW_DEBUG_PRINTS) console.log(a); };				// A log function we can turn off
var exists = function(a) { return typeof(a) == "undefined" ? false : true; };	// Check whether a variable exists
var dflt = function(a, b) { 													// Default a to b if a is undefined
	if( typeof(a) === "undefined" ){ 
		return b; 
	} else return a; 
};


//////////////////////////////////////////////////////////////////////////
// Constructor
function AudioEngine() {
	var audioEngineImpl = require( "./build/Release/NodeCoreAudio" );

	this.audioEngine = audioEngineImpl.createAudioEngine( function() {} );
	
	this.processingCallbacks = [];
	this.uiUpdateCallbacks = [];
	this.didProcessAudio = false;
	
	this.outputBuffer = [];
	this.tempBuffer = [];
	this.processBuffer = [];
	
	// Allocate a processing buffer for each of our channels
	for( var iChannel = 0; iChannel<MAX_SUPPORTED_CHANNELS; ++iChannel ) {
		this.processBuffer[iChannel] = [];
	}
	
	// Start polling the audio engine for data as fast as we can	
	var self = this;
	setInterval( function() {	
		self.didProcessAudio = false;
		
		// Try to process audio
		self.audioEngine.processIfNewData( self.getProcessAudio() );
		
		// If we processed some audio, call our UI updates
		if( self.didProcessAudio ) {
			for( var iUpdate=0; iUpdate < self.uiUpdateCallbacks.length; ++iUpdate ) {
				self.uiUpdateCallbacks[iUpdate]();
			}
		}
	}, 0 );
} // end AudioEngine();


//////////////////////////////////////////////////////////////////////////
// Main audio processing function
AudioEngine.prototype.getProcessAudio = function( numSamples, inputBuffer ) {
	var self = this;
	var numChannels = this.audioEngine.getNumInputChannels();
	
	var processAudio = function( numSamples, inputBuffer ) {		
		// If we don't have any processing callbacks, just get out
		if( self.processingCallbacks.length == 0 )
			return inputBuffer;
			
		var processBuffer = self.processBuffer;
			
		// We need to deinterleave the inputbuffer
		deInterleave( inputBuffer, processBuffer, numSamples, numChannels );

		// Call through to all of our processing callbacks
		for( var iCallback = 0; iCallback < self.processingCallbacks.length; ++iCallback ) {
			processBuffer = self.processingCallbacks[iCallback]( numSamples, processBuffer );
		} // end for each callback
		
		var outputBuffer = self.outputBuffer;
		
		interleave( processBuffer, outputBuffer, numSamples, numChannels );
		
		self.didProcessAudio = true;
		
		// Return our output audio to the sound card
		return outputBuffer;
	} // end processAudio()
	
	return processAudio;
} // end AudioEngine.getProcessAudio()


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
// Splits a 1d buffer into its channel components
function deInterleave( inputBuffer, outputBuffer, numSamplesPerBuffer, numChannels ) {
	for( var iSample = 0; iSample < numSamplesPerBuffer; iSample += numChannels ) {
		for( var iChannel = 0; iChannel < numChannels; ++iChannel ) {
			outputBuffer[iChannel][iSample] = inputBuffer[iSample + iChannel];
		} // end for each channel		
	} // end for each sample position
} // end deInterleave()


//////////////////////////////////////////////////////////////////////////
// Joins multidimensional array into single buffer
function interleave( inputBuffer, outputBuffer, numSamplesPerBuffer, numChannels ) {
	for( var iSample = 0; iSample < numSamplesPerBuffer; iSample += numChannels ) {
		for( var iChannel = 0; iChannel < numChannels; ++iChannel ) {
			outputBuffer[iSample + iChannel] = inputBuffer[iChannel][iSample];
		} // end for each channel		
	} // end for each sample position
} // end interleave()