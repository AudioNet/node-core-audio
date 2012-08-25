//////////////////////////////////////////////////////////////////////////
// AudioHubDSP - Audio related functionality for AudioHub
//////////////////////////////////////////////////////////////////////////
//
// This class holds all processing functions and anything related to audio
/* ----------------------------------------------------------------------
													Object Structures
-------------------------------------------------------------------------
	
*/
//////////////////////////////////////////////////////////////////////////
// Node.js Exports
var globalNamespace = {};
(function (exports) {
	exports.createNewAudioHubDSP = function( isClient ) {
		newAudioHubDSP= new AudioHubDSP( isClient );
		return newAudioHubDSP;
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
function AudioHubDSP( isClient ) {
	this.coreAudio = require("D:/Projects/node-core-audio/node-core-audio").createNewAudioEngine();	
	
	this.io;
	this.socket;
	this.hasNewData = false;
	this.gotNewData = false;
	this.sampleFrames;
	this.tempBuffer = [];
	this.incomingBuffer = [];
	this.currentUser;
	this.loudnessMeter;
	
	var audioEngine = this.coreAudio.audioEngine;
	var io = this.io;
	var socket = this.socket;
	var hasNewData = this.hasNewData;
	var gotNewData = this.gotNewData;
	var sampleFrames = this.sampleFrames;
	var tempBuffer = this.tempBuffer;
	var incomingBuffer = this.incomingBuffer;
	var currentUser = this.currentUser;
	var loudnessMeter = this.loudnessMeter;
	
	if( isClient ) {
		io = require( 'socket.io-client' );
		socket = io.connect('http://localhost:9999');
		loudnessMeter = require("./LoudnessMeterDSP").createNewLoudnessMeter( socket );
		
		socket.on( "connect", function() {
			start = new Date().getTime();
		});
		
		socket.on( "audioData", function( data ) {
			tempBuffer = data.buffer;
			hasNewData = true;
			
			if( !hasEstimatedLatency ) {
				hasEstimatedLatency = true;
				var elapsed = new Date().getTime() - start;
				console.log( "Latency of " + elapsed + " milliseconds" );
			}
		});	
	} else {
		io  = require( 'socket.io' ).listen( 9999 );
		io.set('log level', 1);                    // reduce logging
		
		io.sockets.on( 'connection', function( newSocket ) {
			socket = newSocket;
		
			loudnessMeter = require("./LoudnessMeterDSP").createNewLoudnessMeter( socket );
			console.log( "Socket connection" );
			currentUser = socket.id;
			
			socket.on( "audioData", function( data ) {
				gotNewData = true;
				incomingBuffer = data.buffer;
			});
		});
	}
	
	var util = require( "util" );

	/*
	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});
	*/
	
	
	var audioEngineImpl = require( "D:/Projects/node-core-audio/Test/StreamerApps/AudioHub/data/node_modules/node-core-audio/NodeCoreAudio" );

	console.log( audioEngineImpl );
	
	var sinePhase = 0;
	var sampleRate = audioEngine.getSampleRate();

	// Our processing function
	function processAudio( numSamples, incomingSamples ) {	
		var sum = 0;
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			sum += incomingSamples[iSample];
		}
	
		sampleFrames = numSamples;
		
		if( gotNewData ) {
			gotNewData = false;
			for( var iSample = 0; iSample < numSamples; ++iSample ) {
				tempBuffer[iSample] = ( incomingSamples[iSample] + incomingBuffer[iSample] ) / 2;
			}
		} else {
			tempBuffer = incomingSamples;
		}
		
		hasNewData = true;
		
		//sinePhase = generateSine( numSamples, tempBuffer, 440, sampleRate, sinePhase );
		
		return tempBuffer;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {	
		audioEngine.processIfNewData( processAudio );
		
		if( hasNewData ) {
			hasNewData = false;			
			
			if( typeof(loudnessMeter) != "undefined" )
				loudnessMeter.processLoudness( sampleFrames, tempBuffer );
			
			/*
			if( isClient )
				socket.emit( "audioData", { buffer: tempBuffer } );
			else
				io.sockets.socket(currentUser).emit( "audioData", { buffer: tempBuffer } );
			*/
		}
	}, 0 );
	
} // end AudioHubDSP


//////////////////////////////////////////////////////////////////////////
// Returns the name of a specified device
AudioHubDSP.prototype.getDeviceName = function( deviceId ) {
	return this.coreAudio.audioEngine.getDeviceName( deviceId );
} // end AudioHubDSP.getDeviceName()


//////////////////////////////////////////////////////////////////////////
// Returns the name of a specified device
AudioHubDSP.prototype.getNumDevices = function() {
	return this.coreAudio.audioEngine.getNumDevices();
} // end AudioHubDSP.getNumDevices()


//////////////////////////////////////////////////////////////////////////
// Generates a sine wave
function generateSine( numSamples, buffer, frequency, sampleRate, previousPhase ) {
	var period = 1 / frequency;
	
	var bufferLengthMs = numSamples / sampleRate,	// Length of our buffer in time
		phaseDelta = numSamples / period,			// Change in phase while generating for this buffer
		samplePhaseDelta = phaseDelta / numSamples;	// Change in phase between samples
		
	for( var iSample = 0; iSample < numSamples; ++iSample ) {
		buffer[iSample] = Math.sin( previousPhase + iSample * samplePhaseDelta );
	}
	
	var newPhase = previousPhase + phaseDelta;
	
	// Subtract 2pi until we get inside ( 0 > phase > 2pi )
	while( newPhase > Math.PI * 2 ) {
		newPhase -= Math.PI * 2;
	}
	
	return newPhase;
} // end generateSine()