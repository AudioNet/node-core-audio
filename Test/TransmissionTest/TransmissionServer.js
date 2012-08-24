// Set this to something like 15000 to give yourself some time to attach to the node.exe process from a C++ debugger
var TIME_TO_ATTACH_DEBUGGER_MS = 0;

setTimeout( function() {

	var io  = require( 'socket.io' ).listen( 9999 );
	io.set('log level', 1);                    // reduce logging
	var hasNewData = false;
	var tempBuffer = [];
	var serverBuffer = [];
	var currentUser;
	var tickDelay = 50000;
	var numTillAudio = tickDelay;
	var numTillClientAudio = tickDelay;
	
	io.sockets.on( 'connection', function( socket ) {
		console.log( "Socket connection" );
		currentUser = socket.id;
	});
	
	var util = require( "util" );

	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});

	var audioEngineImpl = require( "NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "some function" );
	});

	// Make sure the audio engine is still active
	if( audioEngine.isActive() ) console.log( "active" );
	else console.log( "not active" );

	// Declare our processing function
	function processAudio( numSamples, incomingSamples ) {
		//console.log( "processing " + numSamples + " samples" );
	
		// Write a saw wave to the buffer
		// It will have f0 = sampleRate/numSamples
		var middleSample = numSamples/2;
		var numRounds = 2;
		
		
		for( var iRound = 0; iRound < numRounds; ++iRound ) {
			for( var iSampleAbsolute = 0; iSampleAbsolute < numSamples/numRounds; ++iSampleAbsolute ) {
				var iSample = iSampleAbsolute * iRound;
				var sawValue = iSample/(numSamples/numRounds);
			
				if( numTillClientAudio == 0 ) {
					numTillClientAudio = tickDelay;
					tempBuffer[iSample] = sawValue;
				} else if( numTillClientAudio > tickDelay - 5000 ) {
					numTillClientAudio -= 1;
					tempBuffer[iSample] = sawValue;
				} else {
					numTillClientAudio -= 1;
					tempBuffer[iSample] = 0;
				}
			}
		}
		
		
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			if( numTillAudio == 0 ) {
				numTillAudio = tickDelay;
				serverBuffer[iSample] = iSample/numSamples;
			} else if( numTillAudio > tickDelay - 5000 ) {
				numTillAudio -= 1;
				serverBuffer[iSample] = iSample/numSamples;
			} else {
				numTillAudio -= 1;
				serverBuffer[iSample] = 0;
			}			
		}
		
		hasNewData = true;
		
		return serverBuffer;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
		
		if( hasNewData ) {
			io.sockets.socket(currentUser).emit( "audioData", { buffer: tempBuffer } );
			hasNewData = false;
		}
	}, 0 );

}, TIME_TO_ATTACH_DEBUGGER_MS );
