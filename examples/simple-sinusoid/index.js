/**
 * This file contains an example of node-core-audio application to generate
 * simple sinusoid with specific frequency.
 * @author Raphael Medaer <rme@escaux.com>
 */

// Imports
var NodeCoreAudio = require('../../');

// Some constants..
var FREQUENCY = 440,
    AMPLITUDE = 0.5;

// Create a core audio engine
var engine = new NodeCoreAudio({
	inputChannels: 1,
	outputChannels: 2,
	interleaved: true,
	useMicrophone: false,
});

// Fetch options from engine
var options = engine.getOptions();

// Initialize useful variables and buffer
var sample = 0;
var output = new Array(options.framesPerBuffer * options.outputChannels);

// Add a callback to generate output audio
engine.addAudioCallback(function() {
	// Generate each samples one-by-one
	for (var i = 0; i < options.framesPerBuffer; i++, sample++) {

		// Generate sample value to make a sinusoid
		// NOTE: Could be optimized, but still for example...
		var value = Math.sin(sample * FREQUENCY * 2 * Math.PI / options.sampleRate) * AMPLITUDE;

		// Put value on each channels
		for (var channel = 0; channel < options.outputChannels; channel++) {
			output[i * options.outputChannels + channel] = value;
		}
	}

	return output;
});
