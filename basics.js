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


var writer = null;

var startaudio = function() {
  var engine, i, j, options;

  engine = coreaudio.createNewAudioEngine();
  engine.addAudioCallback(audioCallback);

  engine.setOptions({ inputChannels: 1, framesPerBuffer: 4096, sampleFormat: 1 });
  options = engine.getOptions();
  console.log('options');
  console.log(options);

  console.log('devices');
  j = engine.getNumDevices();
  for (i = 0; i < j; i++) console.log('#' + i + ': ' + engine.getDeviceName(i));

/* portaudio produces a stream of single-precision IEEE floating-point values in the range of -1..1

   to look like PCM the values should be in the range of 0..1 - hence the (x + 1)/2 transformation in the callback

   once that is done, the raw stream is parseable as

       sox -r 44100 -e floating-point -b 32 -c 1 - ...

   WAV appears to be the easiest container to generate, so we'll use that.

   HOWEVER, UNTIL THIS CODE IS DEBUGGED, WE'RE WRITING TO A FILE...
 */

  writer = fs.createWriteStream('/tmp/a.wav');
  writer.write(waveheader(0, { format: 3, channels: options.inputChannels, sampleRate: options.sampleRate, bitDepth: 32 }));
};

var audioCallback = function(inputBuffer) {
  var i, j, len, pcm;

  for (i = 0; i < inputBuffer.length; i++) {
    len = inputBuffer[i].length;
    pcm = new Buffer(len * 4);

    for (j = 0; j < len; j++) pcm.writeFloatLE((inputBuffer[i][j] + 1.0) / 2.0, j * 4);

    if (!!writer) writer.write(pcm, 'binary');
  }

  for (i = 0; i < inputBuffer.length; i++) for (j = 0; j < inputBuffer[i].length; j++) inputBuffer[i][j] = 0.0;
  return inputBuffer;
};

setTimeout (function() { try { startaudio(); } catch(ex) { console.log('startaudio: ' + ex.message); } }, 0);
