var connect = require('connect'),
	socketPort = 4801,
	httpPort = 4800;

connect.createServer( connect.static(__dirname) ).listen(httpPort);
console.log( "Listening on port: " + httpPort );

var engine = require("./node-core-audio").createNewAudioEngine();
var outputBuffer = [];

var io = require('socket.io').listen(socketPort);

var sockets = [];

io.sockets.on('connection', function (socket) {
	sockets.push( socket );
});

io.set( 'log level', 2 );

var options = {
	inputChannels: 1,
	framesPerBuffer: 4096,
}

engine.setOptions( options );

options = engine.options;

var totalSamples = 0;
var freq = 20000.0;

function windowHamming( coeff, iSample, numTotalCoeffs ) {
	return coeff * 0.08 + 0.46 * ( 1 + Math.cos(2*Math.PI*iSample/numTotalCoeffs) );
}

function windowHanning( coeff, iSample, numTotalCoeffs ) {
	return coeff * 0.5 * ( 1 + Math.cos(2*Math.PI*iSample/numTotalCoeffs) );
}

function getSineSample( iSamp, freq, phase ) {
	var phase = phase || 0.0;

	return Math.sin( Math.PI * freq * 0.5 * iSamp / options.sampleRate );
}

function zero( buffer ) {
	for( var iChannel=0; iChannel<buffer.length; ++iChannel )
		for( var iSample=0; iSample<buffer[iChannel].length; ++iSample )
			buffer[iChannel][iSample] = 0.0;
}

var fftBuffer = [];
engine.addAudioCallback( function(inputBuffer) { 
	outputBuffer = inputBuffer;

	//for( var iSample=0; iSample<outputBuffer[0].length; ++iSample ) {
	//	outputBuffer[0][iSample] = iSample / outputBuffer[0].length;
	//}

	for( var iSample=0; iSample<outputBuffer[0].length; ++iSample ) {
		outputBuffer[0][iSample] = getSineSample( totalSamples, 6000 );
		outputBuffer[0][iSample] += getSineSample( totalSamples, 420 );

		outputBuffer[0][iSample] = windowHamming( outputBuffer[0][iSample], iSample, outputBuffer[0].length );
		totalSamples++;
	}

	engine.fft.simple( fftBuffer, outputBuffer[0], "real" );

	for( var iSocket=0; iSocket<sockets.length; ++iSocket ) {
		sockets[iSocket].emit( "fft", {fft:fftBuffer} );
	}

	zero( inputBuffer );

	return inputBuffer;
});
