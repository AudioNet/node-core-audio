var util = require( "util" );

var audioEngineImpl = require( "../Debug/NodeCoreAudio" );

console.log( audioEngineImpl );

var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
	console.log( "aw shit, we got some samples" );
});

console.log( util.inspect( audioEngineImpl.createAudioEngine ) );
console.log( util.inspect( audioEngine ) );