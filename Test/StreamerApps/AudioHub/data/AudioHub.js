var IS_CLIENT = false;

function runAudioStuff( isClient ) {
	var io;
	var socket;
	var hasNewData = false;
	var gotNewData = false;
	var sampleFrames;
	var tempBuffer = [];
	var incomingBuffer = [];
	var currentUser;
	var loudnessMeter;
	
	if( isClient ) {
		var io = require( 'socket.io-client' );
		socket = io.connect('http://localhost:9999');
		loudnessMeter = require("./LoudnessMeterDSP").createNewLoudnessMeter( socket );
		
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
		
			loudnessMeter = require("./LoudnessMeterDSP").createNewLoudnessMeter( socket );
			console.log( "Socket connection" );
			currentUser = socket.id;
			
			socket.on( "audioData", function( data ) {
				gotNewData = true;
				incomingBuffer = data.buffer;
			});
		});
	}
	
	var util = require( "util" );

	/*
	process.on('uncaughtException', function (err) {
	  console.error(err);
	  console.log("Node NOT Exiting...");
	});
	*/
	
	var audioEngineImpl = require( "../../../../Debug/NodeCoreAudio" );

	console.log( audioEngineImpl );

	var audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
		console.log( "some function" );
	});

	// Our processing function
	function processAudio( numSamples, incomingSamples ) {	
		sampleFrames = numSamples;
		
		if( gotNewData ) {
			gotNewData = false;
			for( var iSample = 0; iSample < numSamples; ++iSample ) {
				tempBuffer[iSample] = ( incomingSamples[iSample] + incomingBuffer[iSample] ) / 2;
			}
		} else {
			tempBuffer = incomingSamples;
		}
		
		hasNewData = true;
		
		return tempBuffer;
	}

	// Start polling the audio engine for data as fast as we can
	setInterval( function() {
		audioEngine.processIfNewData( processAudio );
		
		if( hasNewData ) {
			hasNewData = false;			
			
			if( typeof(loudnessMeter) != "undefined" )
				loudnessMeter.processLoudness( sampleFrames, tempBuffer );
			
			/*
			if( isClient )
				socket.emit( "audioData", { buffer: tempBuffer } );
			else
				io.sockets.socket(currentUser).emit( "audioData", { buffer: tempBuffer } );
			*/
		}
	}, 0 );
}






var app = module.exports = require('appjs');
var path = require('path');

// serves files to browser requests to "http://appjs/*"
app.serveFilesFrom(__dirname + '/assets');

var window = app.createWindow('http://appjs/', {
  width           : 640,
  height          : 460,
  left            : -1,    // optional, -1 centers
  top             : -1,    // optional, -1 centers
  autoResize      : false, // resizes in response to html content
  resizable       : false, // controls whether window is resizable by user
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
  
  $ = window.$

  console.log("Window Ready");
  runAudioStuff( IS_CLIENT );
  
	// Connect to the server
	io = require( "socket.io-client" );
	var socket = io.connect('http://localhost:9999');
	
	// Create our meters
	var loudnessMeter = require("./assets/js/CLoudnessMeter").createNewLoudnessMeter( socket );
});

window.on('close', function(){
  console.log("Window Closed");
});
