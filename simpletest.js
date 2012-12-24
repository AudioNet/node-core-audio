var coreAudio = require("node-core-audio");

// Initialize the audio engine
var output = [];

var options = {
	inputChannels: 1,
	outputChannels: 2
}

var audioCallback = function() {
	console.log("sweet");
}

var engine = coreAudio.createNewAudioEngine( options, audioCallback, true );