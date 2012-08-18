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
		for( var iSample = 0; iSample < numSamples; ++iSample ) {
			incomingSamples[iSample] = iSample/numSamples;
		}
		
		return incomingSamples;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
	}, 0 );

}, 0);