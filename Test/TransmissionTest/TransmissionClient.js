// Set this to something like 15000 to give yourself some time to attach to the node.exe process from a C++ debugger
var TIME_TO_ATTACH_DEBUGGER_MS = 0;

setTimeout( function() {
	var io = require( 'socket.io-client' );
	var socket = io.connect('http://localhost:9999');
	
	console.log( socket ) ;
	
	var tempBuffer;
	var hasNewData = false;
	var start;
	var hasEstimatedLatency = false;
	
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
	
	
	var util = require( "util" );

	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});

	var audioEngineImpl = require( "../../Debug/NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "aw shit, we got some samples" );
	});

	// Make sure the audio engine is still active
	if( audioEngine.isActive() ) console.log( "active" );
	else console.log( "not active" );

	// Declare our processing function
	function processAudio( numSamples, incomingSamples ) {
		
		// Write the incoming samples to the sound card		
		if( hasNewData ) {
			hasNewData = false;
			return tempBuffer;
		} else {
			return incomingSamples;
		}
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
	}, 0 );

}, TIME_TO_ATTACH_DEBUGGER_MS );