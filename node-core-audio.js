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
var log = function( a ) { console.log(a); };
var exists = function(a) { return typeof(a) == "undefined" ? false : true; };	// Check whether a variable exists
var dflt = function(a, b) { 													// Default a to b if a is undefined
	if( typeof(a) === "undefined" ){ 
		return b; 
	} else return a; 
};


//////////////////////////////////////////////////////////////////////////
// Constructor
function AudioEngine() {
	var audioEngineImpl = require( "./lib/NodeCoreAudio" );

	this.audioEngine = audioEngineImpl.createAudioEngine( function() {} );
	
	this.processingCallbacks = [];	
	this.outputBuffer = [];
	this.tempBuffer = [];
	
	// Start polling the audio engine for data as fast as we can
	
	var self = this;
	setInterval( function() {	
		self.audioEngine.processIfNewData( self.getProcessAudio() );
	}, 0 );
} // end AudioEngine();


//////////////////////////////////////////////////////////////////////////
// Main audio processing function
AudioEngine.prototype.getProcessAudio = function( numSamples, inputBuffer ) {

	var self = this;
	var processAudio = function( numSamples, inputBuffer ) {	
		// If we don't have any processing callbacks, just get out
		if( self.processingCallbacks.length == 0 )
			return inputBuffer;
			

		// Grab the buffers we're going to need
		var outputBuffer = self.outputBuffer;
		//var tempBuffer = self.tempBuffer;
		
		outputBuffer = inputBuffer;
		/*
		// Initialize our output buffer to 0's
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			outputBuffer[iSample] = 0;
		}
		*/

		// Call through to all of our processing callbacks
		for( var iCallback = 0; iCallback < self.processingCallbacks.length; ++iCallback ) {
			outputBuffer = self.processingCallbacks[iCallback]( numSamples, outputBuffer );
			/*
			// Add the callback's output samples into our output buffer
			// Normalize by dividing by the number of audio callbacks we have, otherwise
			// the audio will get very loud very quickly
			for( var iSample = 0; iSample < numSamples; ++iSample ) {
				outputBuffer[iSample] += tempBuffer[iSample] / self.processingCallbacks.length;
			}
			*/
		} // end for each callback
		
		// Return our output audio to the sound card
		return outputBuffer;
	} // end processAudio()
	
	return processAudio;
} // end AudioEngine.getProcessAudio()


//////////////////////////////////////////////////////////////////////////
// Add a processing callback 
AudioEngine.prototype.addAudioCallback = function( callback ) {
	this.processingCallbacks.push( callback );
} // end AudioEngine.processAudio()


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