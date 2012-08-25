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

	this.audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "some function" );
	});
	
	this.processingCallbacks = [];	
	this.outputBuffer = [];
	this.tempBuffer = [];
} // end AudioEngine();


//////////////////////////////////////////////////////////////////////////
// Main audio processing function
AudioEngine.prototype.processAudio = function( numSamples, inputBuffer ) {
	// If we don't have any processing callbacks, just get out
	if( this.processingCallbacks.length == 0 )
		return inputBuffer;

	// Grab the buffers we're going to need
	var outputBuffer = this.outputBuffer;
	var tempBuffer = this.tempBuffer;
	
	// Initialize our output buffer to 0's
	for( var iSample = 0; iSample < numSamples; ++iSample ) {
		outputBuffer[iSample] = 0;
	}

	// Call through to all of our processing callbacks
	for( var iCallback = 0; iCallback < this.processingCallbacks.length; ++iCallback ) {
		tempBuffer = this.processingCallbacks[iCallback]( numSamples, inputBuffer );
		
		// Add the callback's output samples into our output buffer
		// Normalize by dividing by the number of audio callbacks we have, otherwise
		// the audio will get very loud very quickly
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			outputBuffer[iSample] += tempBuffer[iSample] / this.processingCallbacks.length;
		}
	} // end for each callback
	
	// Return our output audio to the sound card
	return outputBuffer;
} // end AudioEngine.processAudio()


//////////////////////////////////////////////////////////////////////////
// Add a processing callback 
AudioEngine.prototype.addAudioCallback = function( callback ) {
	this.processingCallbacks.push( callback );
} // end AudioEngine.processAudio()