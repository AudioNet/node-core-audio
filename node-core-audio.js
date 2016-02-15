'use strict'

// Setup
const MAX_SUPPORTED_CHANNELS = 6
const audioEngineMethods = [
  'read',
  'write',
  'isActive',
  'getSampleRate',
  'getInputDeviceIndex',
  'getOutputDeviceIndex',
  'getDeviceName',
  'getNumDevices',
  'setInputDevice',
  'setOutputDevice',
  'getNumInputChannels',
  'getNumOutputChannels'
]

// Exports
module.exports = {
  createNewAudioEngine: (options) => new AudioEngine(options)
}

// AudioEngine constructor
function AudioEngine (options) {
  let self = this
  let audioEngineImpl = require(__dirname + '/build/Release/NodeCoreAudio')
  options = options || {
    inputChannels: 1,
    outputChannels: 2,
    framesPerBuffer: 1024,
    useMicrophone: true
  }
  self.audioEngine = audioEngineImpl.createAudioEngine(options)
  // map audioEngine instance methods
  audioEngineMethods.forEach((methodName) => {
    self[methodName] = (args) => Array.isArray(args)
      ? self.audioEngine[methodName](...args)
      : self.audioEngine[methodName](args)
  })
  self.options = self.audioEngine.getOptions()
  self.processingCallbacks = []
  self.uiUpdateCallbacks = []
  self.outputBuffer = []
  self.tempBuffer = []
  self.processBuffer = []

  function validateOutputBufferStructure (buffer) {
    // verify we have a buffer
    if (buffer === undefined) {
      console.log('Audio processing function didn\'t return an output buffer')
      return false
    }
    // verify input channel count
    if (buffer.length !== self.options.inputChannels) {
      console.log(
        (buffer.length > self.options.inputChannels)
          ? 'Output buffer has info for too many channels'
          : 'Output buffer doesn\'t have data for enough channels'
        )
      return false
    }
    // verify buffer types
    let isBufferArray = (typeof (buffer[0]) !== 'object')
    if (isBufferArray || typeof (buffer[0][0]) !== 'number') {
      console.log(
        (isBufferArray)
          ? 'Output buffer not setup correctly, buffer[0] isn\'t an array'
          : 'Output buffer not setup correctly, buffer[0][0] isn\'t a number'
        )
      return false
    }
    return true
  }

  // Allocate a processing buffer for each of our channels
  for (let iChannel = 0; iChannel < MAX_SUPPORTED_CHANNELS; ++iChannel) {
    self.processBuffer[iChannel] = []
  }

  // Start polling the audio engine for data as fast as we can
  self.processAudio = self.getProcessAudio()

  let processBuffer = () => {
    if (self.audioEngine.isBufferEmpty()) {
      // Try to process audio
      let input = self.audioEngine.read()
      let outputBuffer = self.processAudio(input)
      if (validateOutputBufferStructure(outputBuffer)) {
        self.audioEngine.write(outputBuffer)
      }
      // Call our UI updates now that all the DSP work has been done
      self.uiUpdateCallbacks.forEach((cb) => cb())
    }
  }
  setInterval(processBuffer, 1)
}

// Returns our main audio processing function
AudioEngine.prototype.getProcessAudio = function () {
  let self = this
  let options = self.audioEngine.getOptions()
  let numChannels = options.inputChannels

  let processAudio = function (inputBuffer) {
    // If we don't have any processing callbacks, just get out
    if (self.processingCallbacks.length === 0) return inputBuffer
    let processBuffer = inputBuffer
    // Call through to all of our processing callbacks
    self.processingCallbacks.forEach((cb) => processBuffer = cb(processBuffer))
    if (self.audioStreamer !== undefined) {
      self.audioStreamer.streamAudio(
        processBuffer, self.options.framesPerBuffer, numChannels
      )
    }
    // Return our output audio to the sound card
    return processBuffer
  }
  return processAudio
}

// Get the engine's options
AudioEngine.prototype.getOptions = function () {
  this.options = this.audioEngine.getOptions()
  return this.options
}

// Get the engine's options
AudioEngine.prototype.setOptions = function (options) {
  this.audioEngine.setOptions(options)
  this.options = this.audioEngine.getOptions()
}

// Add a processing callback
AudioEngine.prototype.addAudioCallback = function (callback) {
  this.processingCallbacks.push(callback)
}

// Add a UI update callback
AudioEngine.prototype.addUpdateCallback = function (callback) {
  this.uiUpdateCallbacks.push(callback)
}
