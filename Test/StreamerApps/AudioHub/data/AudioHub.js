var IS_CLIENT = false;

var audioEngine;

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

	audioEngine = audioEngineImpl.createAudioEngine( function(uSampleFrames, inputBuffer, outputBuffer) {
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

app.serveFilesFrom(__dirname + '/assets');

var window = app.createWindow({
  width           : 640,
  height          : 460,
  resizable: false,
  disableSecurity: true,
  icons: __dirname + '/assets/icons'
});


window.on('create', function(){
	window.frame.show();
	window.frame.center();
});

window.on('ready', function(){
	this.require = require;
	this.process = process;
	this.module = module;
	this.console.log('process', process);
	this.frame.center();
	this.frame.show();
  
	var $ = window.$;
	
	var $inputDevices = $('#combobox');
	var inputDeviceCombo = $('#combobox').combobox();
	$inputDevices.combobox.onChange = function() {
		console.log( "woot" );
	}; 
	
	console.log("Window Ready");
	runAudioStuff( IS_CLIENT );
	
	var deviceNames = getDeviceNames();
	for( iDevice = 0; iDevice < deviceNames.length; ++iDevice ) {
		$inputDevices.append("<option value=" + deviceNames[iDevice] + ">" + deviceNames[iDevice] + "</option>");
	}
  
	// Connect to the server
	io = require( "socket.io-client" );
	var socket = io.connect('http://localhost:9999');
	
	// Create our meters
	var loudnessMeter = require("./assets/js/CLoudnessMeter").createNewLoudnessMeter( $, socket );
});

window.on('close', function(){
	console.log("Window Closed");
});

function getDeviceNames() {
	var deviceNames = [];

	for( var iDevice = 0; iDevice < audioEngine.getNumDevices(); ++iDevice ) {
		deviceNames.push( audioEngine.getDeviceName( iDevice ) );
	}
	
	return deviceNames;
} // end getDeviceNames()

function onInputDeviceChange() {
	console.log( "hello" );
}