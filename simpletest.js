process.on('uncaughtException', function (err) {

  console.log('Caught exception: ' + err);
  var ares = require("ares").ares;

  ares("node -v", true );
  ares("which node", true );
  setTimeout( function(){}, 200000 );
});

var coreAudio = require("./node-core-audio");

var options = {
	inputChannels: 1,
	outputChannels: 2
}

var audioCallback = function( numSamples, inputBuffer ) {
	console.log("sweet");
	return inputBuffer;
}

console.log( options );

setTimeout( function(){
	console.log("About to create the audio engine" );
	var engine = coreAudio.createNewAudioEngine();

	engine.addAudioCallback( audioCallback );
}, 0);