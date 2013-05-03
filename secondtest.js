var engine = require("./node-core-audio").createNewAudioEngine();
var outputBuffer = [];

engine.addAudioCallback( function(inputBuffer) { 
	outputBuffer = inputBuffer[0];

	for( var iSample=0; iSample<outputBuffer[0].length; ++iSample ) {
		outputBuffer[0][iSample] = iSample / outputBuffer[0].length;
	}

	return outputBuffer;
});