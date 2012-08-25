var IS_CLIENT = false;

var audioEngine;

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
	window.onInputDeviceChange = function( inputDeviceComo ) { onInputDeviceChange( inputDeviceComo.selectedIndex ); };
	
	this.DSP = require("./AudioHubDSP").createNewAudioHubDSP( IS_CLIENT );
	
	
	init( $ );
});


window.on('close', function(){
	console.log("Window Closed");
});


function init( $ ) {	
	// Connect to the server
	io = require( "socket.io-client" );
	var socket = io.connect('http://localhost:9999');
	
	$( "#slider" ).slider();

	var $inputDevices = $('#combobox');
	//var inputDeviceCombo = $('#combobox').combobox();
	$inputDevices.combobox.onChange = function() {
		console.log( "woot" );
	}; 
	
	var deviceNames = getDeviceNames();
	for( iDevice = 0; iDevice < deviceNames.length; ++iDevice ) {
		$inputDevices.append("<option value=" + iDevice + ">" + deviceNames[iDevice] + "</option>");
	}
	
	// Create our meters
	var loudnessMeter = require("./assets/js/CLoudnessMeter").createNewLoudnessMeter( $, socket );
	
	console.log("Window Ready");
} // end initUI()


function getDeviceNames() {
	var audioEngine = window.DSP.coreAudio.audioEngine;
	var deviceNames = [];

	for( var iDevice = 0; iDevice < audioEngine.getNumDevices(); ++iDevice ) {
		deviceNames.push( window.DSP.getDeviceName( iDevice ) );
	}
	
	return deviceNames;
} // end getDeviceNames()


function onInputDeviceChange( inputDevice ) {
	var audioEngine = window.DSP.coreAudio.audioEngine;
	
	console.log( "Combo box changed to input " + audioEngine.getDeviceName(inputDevice) );
	
	try {
		audioEngine.setInputDevice( inputDevice );
	} catch ( error ) {
		console.log( error );
	}
} // end onInputDeviceChange()