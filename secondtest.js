var engine = require("./node-core-audio").createNewAudioEngine();
var outputBuffer = [];

var options = {
	inputChannels: 1,
	framesPerBuffer: 512,
}

var hasdone = false;

//engine.setOptions( options );

engine.addAudioCallback( function(inputBuffer) { 
	outputBuffer = inputBuffer;

	for( var iSample=0; iSample<outputBuffer[0].length; ++iSample ) {
		outputBuffer[0][iSample] = iSample / outputBuffer[0].length;
	}

	//console.log( inputBuffer.length + " channels" );
	//console.log( inputBuffer[0].length + " samples" );
	//console.log( outputBuffer.length + " channels" );
	//console.log( outputBuffer[0].length + " samples" );

	if( !hasdone ) {
		console.log( inputBuffer );
		hasdone = true;
	}

	return outputBuffer;
});