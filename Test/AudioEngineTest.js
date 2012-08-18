var util = require( "util" );

process.on('uncaughtException', function (err) {
  console.error(err);
  console.log("Node NOT Exiting...");
});

var audioEngineImpl = require( "../Debug/NodeCoreAudio" );

console.log( audioEngineImpl );

var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
	console.log( "aw shit, we got some samples" );
});

console.log( util.inspect( audioEngineImpl.createAudioEngine ) );
console.log( util.inspect( audioEngine ) );

//console.log( audioEngine.plusOne( function(data){console.log(data);}) );

console.log( "alive");

if( audioEngine.isActive() ) console.log( "active" );
else console.log( "not active" );

var http = require('http');
http.createServer(function (req, res) {
  res.writeHead(200, {'Content-Type': 'text/plain'});
  res.end('Hello World\n');
}).listen(1337, '127.0.0.1');
console.log('Server running at http://127.0.0.1:1337/');