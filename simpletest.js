var coreAudio = require("./node-core-audio");

var options = {
	inputChannels: 1,
	outputChannels: 2
}

var audioCallback = function() {
	console.log("sweet");
}

setTimeout( function(){
	var engine = coreAudio.createNewAudioEngine( options, audioCallback, true );
}, 0);