setTimeout( function() {

	var util = require( "util" );

	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});

	var audioEngineImpl = require( "../Debug/NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "aw shit, we got some samples" );
	});

	// Make sure the audio engine is still active
	if( audioEngine.isActive() ) console.log( "active" );
	else console.log( "not active" );

	// Declare our processing function
	function processAudio( numSamples, incomingSamples ) {
		console.log( incomingSamples );
	}

	// Start polling the audio engine for data every 2 milliseconds
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
	}, 2 );

}, 15000);