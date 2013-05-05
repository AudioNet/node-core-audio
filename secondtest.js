var engine = require("./node-core-audio").createNewAudioEngine();
var outputBuffer = [];

var options = {
	inputChannels: 1,
	framesPerBuffer: 1024,
}

engine.setOptions( options );

engine.addAudioCallback( function(inputBuffer) { 
	outputBuffer = inputBuffer;

	for( var iSample=0; iSample<outputBuffer[0].length; ++iSample ) {
		outputBuffer[0][iSample] = iSample / outputBuffer[0].length;
	}

	return outputBuffer;
});