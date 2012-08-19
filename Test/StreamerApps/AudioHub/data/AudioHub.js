var IS_CLIENT = true;

function runAudioStuff( isClient ) {
	var io;
	var socket;
	var hasNewData = false;
	var gotNewData = false;
	var tempBuffer = [];
	var incomingBuffer = [];
	var currentUser;
	
	if( isClient ) {
		var io = require( 'socket.io-client' );
		socket = io.connect('http://localhost:9999');
		
		socket.on( "connect", function() {
			start = new Date().getTime();
		});
		
		socket.on( "audioData", function( data ) {
			tempBuffer = data.buffer;
			hasNewData = true;
			
			if( !hasEstimatedLatency ) {
				hasEstimatedLatency = true;
				var elapsed = new Date().getTime() - start;
				console.log( "Latency of " + elapsed + " milliseconds" );
			}
		});	
	} else {
		io  = require( 'socket.io' ).listen( 9999 );
		io.set('log level', 1);                    // reduce logging
		
		io.sockets.on( 'connection', function( newSocket ) {
			socket = newSocket;
			console.log( "Socket connection" );
			currentUser = socket.id;
			
			socket.on( "audioData", function( data ) {
				gotNewData = true;
				incomingBuffer = data.buffer;
			});
		});
	}
	
	var util = require( "util" );

	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});

	var audioEngineImpl = require( "../../../../Debug/NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "some function" );
	});

	// Make sure the audio engine is still active
	if( audioEngine.isActive() ) console.log( "active" );
	else console.log( "not active" );

	// Our processing function
	function processAudio( numSamples, incomingSamples ) {	
		
		if( gotNewData ) {
			gotNewData = false;
			for( var iSample = 0; iSample < numSamples; ++iSample ) {
				tempBuffer[iSample] = ( incomingSamples[iSample] + incomingBuffer[iSample] ) / 2;
			}
		} // end if we got new data
		
		hasNewData = true;
		
		return tempBuffer;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
		
		if( hasNewData ) {
			hasNewData = false;			
			if( isClient )
				socket.emit( "audioData", { buffer: tempBuffer } );
			else
				io.sockets.socket(currentUser).emit( "audioData", { buffer: tempBuffer } );
		}
	}, 0 );
}






var app = module.exports = require('appjs');
var path = require('path');

// serves files to browser requests to "http://appjs/*"
app.serveFilesFrom(path.resolve(__dirname, 'content'));

var window = app.createWindow('http://appjs/', {
  width           : 640,
  height          : 460,
  left            : -1,    // optional, -1 centers
  top             : -1,    // optional, -1 centers
  autoResize      : false, // resizes in response to html content
  resizable       : true, // controls whether window is resizable by user
  showChrome      : true,  // show border and title bar
  opacity         : 1,     // opacity from 0 to 1 (Linux)
  alpha           : true,  // alpha composited background (Windows & Mac)
  fullscreen      : false, // covers whole screen and has no border
  disableSecurity : true   // allow cross origin requests
});


window.on('create', function(){
  console.log("Window Created");
});

window.on('ready', function(){
  this.require = require;
  this.process = process;
  this.module = module;
  this.console.log('process', process);
  this.frame.center();
  this.frame.show();
  console.log("Window Ready");
  runAudioStuff( IS_CLIENT );
});

window.on('close', function(){
  console.log("Window Closed");
});
