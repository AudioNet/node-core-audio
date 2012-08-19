//////////////////////////////////////////////////////////////////////////
// LoudnessMeter - Server Side
//////////////////////////////////////////////////////////////////////////
//
// Recieves and processes data for the loudness meter
/* ----------------------------------------------------------------------
													Object Structures
-------------------------------------------------------------------------
	
*/
//////////////////////////////////////////////////////////////////////////
// Node.js Exports
var globalNamespace = {};
(function (exports) {
	exports.createNewLoudnessMeter = function( socket ) {
		newLoudnessMeter= new LoudnessMeter( socket );
		return newLoudnessMeter;
	};
}(typeof exports === 'object' && exports || globalNamespace));


//////////////////////////////////////////////////////////////////////////
// Namespace (lol)
var log = function( a ) { console.log(a); };
var exists = function(a) { return typeof(a) == "undefined" ? false : true; };	// Check whether a variable exists
var dflt = function(a, b) { 													// Default a to b if a is undefined
	if( typeof(a) === "undefined" ){ 
		return b; 
	} else return a; 
};


//////////////////////////////////////////////////////////////////////////
// Constructor
function LoudnessMeter( socket ) {
	this.socket = socket;
	this.tempSum = 0;
} // end LoudnessMeter();


//////////////////////////////////////////////////////////////////////////
// Returns the width of the canvas
LoudnessMeter.prototype.processLoudness = function( numSamples, audioBuffer ) {
	// Calculate average loudness in this buffer
	this.tempSum = 0;
	for( var iSample = 0; iSample < numSamples; ++iSample ) {
		this.tempSum += audioBuffer[iSample];
	}
	
	var averageLoudness = this.tempSum / numSamples;

	this.socket.emit( "newLoudnessData", {loudness: averageLoudness} );
} // end LoudnessMeter.getWidth()