// just the basics
    
var coreaudio  = require('./node-core-audio')
  , http       = require('http')
  , portfinder = require('portfinder')
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
      response.writeHead(200, { 'Content-Type': 'audio', 'Content-Transfer-Encoding': 'binary' });
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


var startaudio = function() {
  var engine, i, j;
    
  engine = coreaudio.createNewAudioEngine();
  engine.addAudioCallback( audioCallback );
    
  console.log('isActive: ' + (engine.isActive() ? 'true' : 'false'));
  console.log('options');
  console.log(engine.getOptions ());
  console.log('devices');
  j = engine.getNumDevices();
  for (i = 0; i < j; i++) console.log('#' + i + ': ' + engine.getDeviceName(i));
};
    
var audioCallback = function(inputBuffer) {
// TBD: loop through clients[] and send the buffer to each
//      howeer, there's a few questions for mike (-;

  return inputBuffer;
};

setTimeout (function() { try { startaudio(); } catch(ex) { console.log('startaudio: ' + ex.message); } }, 0);
