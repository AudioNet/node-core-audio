// just the basics

var coreaudio  = require('./node-core-audio')
  , fs         = require('fs')
  , http       = require('http')
  , portfinder = require('portfinder')
  , waveheader = require('waveheader')
  ;


process.on('uncaughtException', function(err) {
  console.log('process: ' + err.message);
  console.error(err);
});


var clients = [];

portfinder.getPort({ port: 9999 }, function(err, portno) {
  if (err) return console.log('portfinder.getPort 9999: ' + err.message);

  var addr = function(socket) {
    var sockaddr = socket.address ();

    return (sockaddr.address + ':' + sockaddr.port);
  };

  var remove = function(socket) {
    var i;

    for (i = 0; i < clients.length; i++) {
      if (clients[i] === socket) {
        clients = clients.splice(i, 1);
        return;
      }
    }
  };

  http.createServer(function(request, response) {
    if (request.method !== 'GET') {
      response.writeHead(405, { Allow: 'GET' });
      return response.end();
    }

// ignore any body....
    request.on('end', function() {
      response.writeHead(200, { 'Content-Type': 'audio/wav', 'Content-Transfer-Encoding': 'binary' });
      response.write(new Buffer([ ]));
      clients.push(response.socket);
    }).on('close', function() {
      console.log('client close: ' + addr(request.socket));
      remove(request.socket);
    });
  }).on('clientError', function(err, socket) {
    console.log('client error: ' + err.message + '[' + addr(socket) + ']');
    remove(socket);
  }).on('listening', function() {
    console.log('listening on port ' + portno);
  }).listen(portno);
});


var inputDevice  = 'Soundflower (2ch)';
var outputDevice = 'Built-in Output';
var sampleFormat  = 'int32';

var sampleFormats = { float32 : { format: 0x01, bits: 32, write: 'writeFloatLE' }
                    , int32   : { format: 0x02, bits: 32, write: 'writeInt32LE' }
                    , int16   : { format: 0x08, bits: 16, write: 'writeInt16LE' }
                    , int8    : { format: 0x10, bits:  8, write: 'writeInt8'    }
                    , uint8   : { format: 0x20, bits:  8, write: 'writeUInt8'   }
                    };
var options = null;
var writer = null;

var startaudio = function() {
  var device, engine, i, input, j, output;

  engine = coreaudio.createNewAudioEngine();
  engine.addAudioCallback(audioCallback);

  console.log('devices');
  for (i = 0, j = engine.getNumDevices(); i < j; i++) {
    device = engine.getDeviceName(i);
    console.log('#' + i + ': ' + device);

    if (device.indexOf(inputDevice) === 0) input = i;
    if (device.indexOf(outputDevice) === 0) output = i;
  }
  if (!input)  { console.log('unable to find "' + inputDevice + '"');  process.exit(1); }
  if (!output) { console.log('unable to find "' + outputDevice + '"'); process.exit(1); }

console.log(sampleFormats[sampleFormat]);
  engine.setOptions({ inputChannels   : 2
                    , outputChannels  : 2
                    , inputDevice     : input
                    , outputDevice    : output
                    , framesPerBuffer : 4096
                    , sampleFormat    : sampleFormats[sampleFormat].format
                    , interleaved     : true
                    });
  options = engine.getOptions();
  console.log('options');
  console.log(options);

/* UNTIL THIS CODE IS DEBUGGED, WE'RE WRITING TO A FILE... */
  writer = fs.createWriteStream('/tmp/a.wav');
  writer.write(waveheader(0, { format     : (options.sampleFormat === sampleFormats.float32.format) ? 3 : 1
                             , channels   : options.inputChannels
                             , sampleRate : options.sampleRate
                             , bitDepth   : sampleFormats[sampleFormat].bits
                             }));
};

var audioCallback = function(inputBuffer) {
  var depth, i, j, len, pcm;

  if (!!writer) {
    depth = sampleFormats[sampleFormat].bits / 8;

    if (options.interleaved) {
      len = inputBuffer.length;
      pcm = new Buffer(len * depth);

      if (options.sampleFormat === sampleFormats.float32.format) {
        for (j = 0; j < len; j++) pcm.writeFloatLE((inputBuffer[j] + 1.0) / 2.0, j * depth);
      } else {
        for (j = 0; j < len; j++) pcm[sampleFormats[sampleFormat].write](inputBuffer[j], j * depth);
      }

      writer.write(pcm, 'binary');
    } else {
      for (i = 0; i < inputBuffer.length; i++) {
        len = inputBuffer[i].length;
        pcm = new Buffer(len * depth);

        if (options.sampleFormat === sampleFormats.float32.format) {
          for (j = 0; j < len; j++) pcm.writeFloatLE((inputBuffer[i][j] + 1.0) / 2.0, j * depth);
        } else {
          for (j = 0; j < len; j++) pcm[sampleFormats[sampleFormat].write](inputBuffer[i][j], j * depth);
        }

        writer.write(pcm, 'binary');
      }
    }
  }

  if (options.sampleFormat === sampleFormats.float32.format) {
    for (i = 0; i < inputBuffer.length; i++) for (j = 0; j < inputBuffer[i].length; j++) inputBuffer[i][j] = 0.0;
  } else {
    for (i = 0; i < inputBuffer.length; i++) for (j = 0; j < inputBuffer[i].length; j++) inputBuffer[i][j] = 0;
  }
  return inputBuffer;
};

setTimeout (function() { try { startaudio(); } catch(ex) { console.log('startaudio: ' + ex.message); } }, 0);
