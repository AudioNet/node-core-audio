// Set this to something like 15000 to give yourself some time to attach to the node.exe process from a debugger
var TIME_TO_ATTACH_DEBUGGER_MS = 0;

setTimeout( function() {

	var util = require( "util" );

	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});

	var audioEngineImpl = require( "NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "aw shit, we got some samples" );
	});

	// Make sure the audio engine is still active
	if( audioEngine.isActive() ) console.log( "active" );
	else console.log( "not active" );

	// Declare our processing function
	function processAudio( numSamples, incomingSamples ) {
		// Write a saw wave to the buffer
		// It will have f0 = sampleRate/numSamples
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			incomingSamples[iSample] = iSample/numSamples;
		}
		
		return incomingSamples;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
	}, 0 );

}, TIME_TO_ATTACH_DEBUGGER );