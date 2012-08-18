var audioEngineImpl = require( "../Debug/NodeCoreAudio" );

var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
	console.log( "aw shit, we got some samples" );
});

console.log( audioEngine );