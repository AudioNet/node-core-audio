var CoreAudio = require( __dirname + '/build/Release/NodeCoreAudio');
var EventEmitter = require('events').EventEmitter;
var util = require('util');

var DEFAULT_OPTIONS = {
	inputChannels: 1,
	outputChannels: 2,
	framesPerBuffer: 1024,
	useMicrophone: true
};

/**
 * AudioEngine binding for portaudio library.
 * @constructor
 * @param {object} options - Audio stream options.
 * @param {boolean} debug - Enable debugging (disabled by default).
 */
function AudioEngine(options, debug) {
	var self = this;

	// Since we inherits EventEmitter, call super constructor
	EventEmitter.call(this);

	this.debug = (typeof debug === 'undefined' ? false : debug);

	// Initialize the native audio engine
	this.engine = new CoreAudio.AudioEngine(options || DEFAULT_OPTIONS);

	// Processing callbacks
	this.callbacks = [];

	// Start polling the audio engine for data as fast as we can
	setInterval(function() {
		// Skip this turn if buffer is not empty
		if (! self.engine.isBufferEmpty()) return;

		// Try to process audio
		var buffer = self.processAudio(self.engine.read());
		if (self.validateOutputBuffer(buffer))
			self.engine.write(buffer);

		// Notify we updated audio engine buffer
		EventEmitter.prototype.emit.call(self, 'updated');
	}, 1);
}

// Inherits EventEmitter
util.inherits(AudioEngine, EventEmitter);

/**
 * Validate output buffer structure.
 * @param {array} buffer - The buffer to validate.
 * @return {boolean} - True if valid, or false.
 */
AudioEngine.prototype.validateOutputBuffer = function(buffer) {
	// If buffer is not set or is not an array
	if (typeof buffer === 'undefined' || ! Array.isArray(buffer)) {
		this.log('Audio processing function didn\'t return a valid output buffer');
		return false;
	}

	var options = this.engine.getOptions();

	if (! options.interleaved) {
		// TODO we only validate inputChannels ... We should compare also
		// to outputChannels ..
		if (buffer.length > options.inputChannels) {
			this.log('Output buffer has info for too many channels');
			return false;
		} else if (buffer.length < options.inputChannels) {
			this.log('Output buffer doesn\'t have data for enough channels');
			return false;
		}

		// It MUST be a 2 dimensional array ...
		if (! Array.isArray(buffer[0])) {
			this.log('Output buffer not setup correctly, buffer[0] isn\'t an array');
			return false;
		}

		// ... and this array should contain integers.
		if(typeof buffer[0][0] !== 'number') {
			this.log('Output buffer not setup correctly, buffer[0][0] isn\'t a number');
			return false;
		}
	} else {
		if (typeof buffer[0]  !== 'number') {
			this.log('Output buffer not setup correctly, buffer[0] isn\'t a number');
			return false;
		}
	}

	return true;
};

/**
 * Log an AudioEngine message to stdout.
 * @param {string} message - Message to log.
 */
AudioEngine.prototype.log = function(message) {
	this.debug && console.log(message);
};

/**
 * Process each audio through every callbacks.
 * @param {array} buffer - Input buffer.
 * @return {array} - Output buffer.
 */
AudioEngine.prototype.processAudio = function(buffer) {
	var options = this.engine.getOptions();

	// Pipe buffer if not any callback
	if (! this.callbacks.length) return buffer;

	// Call each processing callbacks with buffer
	for (var index = 0; index < this.callbacks.length; index++) {
		buffer = this.callbacks[index](buffer);
	}

	return buffer;
};

/**
 * Get engine's options.
 * @return {object} - Engine's options.
 */
 AudioEngine.prototype.getOptions = function() {
	return this.engine.getOptions();
};

/**
 * Set engine's options.
 * @todo Can we change options in live streaming.. to test !
 * @param {object} options - Engine's options to set.
 */
AudioEngine.prototype.setOptions = function(options) {
	this.engine.setOptions(options);
};

/**
 * Add an audio processing callback.
 * @param {function} callback - A handler with prototype <buffer function(buffer)>
 */
AudioEngine.prototype.addAudioCallback = function(callback) {
	this.callbacks.push(callback);
};

/**
 * Returns whether the audio engine is active.
 * @return {boolean}
 */
AudioEngine.prototype.isActive = function() {
	return this.engine.isActive();
};

/**
 * Returns the name of a given device ID.
 * @param {integer} id - Device ID.
 * @return {string} - Name of the device.
 */
AudioEngine.getDeviceName = function(id) {
	return CoreAudio.AudioEngine.getDeviceName(id);
};

/**
 * Returns the total number of audio devices.
 * @return {integer} - Number of devices.
 */
AudioEngine.getNumDevices = function() {
	return CoreAudio.AudioEngine.getNumDevices();
};

/**
 * Sets the input audio device.
 * @param {integer} id - Device ID.
 */
AudioEngine.prototype.setInputDevice = function(id) {
	return this.engine.setInputDevice(id);
};

/**
 * Sets the output audio device.
 * @param {integer} id - Device ID.
 */
AudioEngine.prototype.setOutputDevice = function(id) {
	return this.engine.setOutputDevice(id);
};

/**
 * Returns the number of input channels.
 * @return {integer} - Number of input channels.
 */
AudioEngine.prototype.getNumInputChannels = function() {
	return this.engine.getNumInputChannels();
};

/**
 * Returns the number of output channels.
 * @return {integer} - Number of output channels.
 */
AudioEngine.prototype.getNumOutputChannels = function() {
	return this.engine.getNumOutputChannels();
};

/**
 * Read audio samples from the sound card.
 * @return {array} - Next samples buffer.
 */
AudioEngine.prototype.read = function() {
	return this.engine.read();
}

/**
 * Write some audio samples to the sound card.
 */
AudioEngine.prototype.write = function() {
	this.engine.write();
};


/**
 * Splits a 1d buffer into its channel components.
 * @todo Refactor ... again !
 * @param {array} inputBuffer - Input buffer.
 * @param {array} outputBuffer - Output buffer.
 * @param {integer} samplesPerBuffer - Number of samples per buffer.
 * @param {integer} channels - Number of channels.
 */
AudioEngine.deInterleave = function(inputBuffer, outputBuffer, samplesPerBuffer, channels) {
	// If the number of channels doesn't match, setup the output buffer
	if (inputBuffer.length !== outputBuffer.length) {
		outputBuffer = [];
		for (var channel = 0; channel < inputBuffer.length; channel++) {
			outputBuffer[channel] = [];
		}
	}

	if (channels < 2) {
		outputBuffer[0] = inputBuffer;
		return;
	}

	// TODO refactor with buffer[sample * channels + channel] = ...
	for (var channel = 0; channel < channels; channel += channels) {
		for (var sample = 0; sample < samplesPerBuffer; ++sample) {
			outputBuffer[channel][sample] = inputBuffer[sample + channel];
		}
	}
};

/**
 * Joins multidimensional array into single buffer.
 * @todo Refactor ... again !
 * @param {array} inputBuffer - Input buffer.
 * @param {array} outputBuffer - Output buffer.
 * @param {integer} samplesPerBuffer - Number of samples per buffer.
 * @param {integer} channels - Number of channels.
 */
AudioEngine.interleave = function(inputBuffer, outputBuffer, samplesPerBuffer, channels) {
	if (channels < 2) {
		outputBuffer = inputBuffer;
		return;
	}

	// If the number of channels doesn't match, setup the output buffer
	if (inputBuffer.length !== outputBuffer.length) {
		outputBuffer = [];
		for (var channel = 0; channel < inputBuffer.length; channel++) {
			outputBuffer[channel] = [];
		}
	}

	for (var channel = 0; channel < channels; channel++) {
		if(typeof inputBuffer[channel] === 'undefined') break;

		for(var sample = 0; sample < samplesPerBuffer; sample += channels ) {
			outputBuffer[sample + channel] = inputBuffer[channel][sample];
		}
	}

};


module.exports = AudioEngine;
