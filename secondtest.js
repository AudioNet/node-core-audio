var engine = require("./node-core-audio").createNewAudioEngine();

engine.addAudioCallback( function(numSamples, inputBuffer) { 
	//console.log( inputBuffer[0] );
	var outputThing = new Array();

	outputThing = inputBuffer;

	console.log( outputThing.length + " channels" );
	console.log( outputThing[0].length + " samples" );
	console.log( outputThing[1].length + " samples" );


	for( var iChannel=0; iChannel<outputThing.length; ++iChannel ) {

		for( var iSample=0; iSample<outputThing[iChannel].length; ++iSample ) {

			if( iSample % 2 )
				outputThing[iChannel][iSample] = 1.0;
			else 
				outputThing[iChannel][iSample] = -1.0;
		}
	}

	return outputThing;
});