//////////////////////////////////////////////////////////////////////////
// LoudnessMeter - Client Side
//////////////////////////////////////////////////////////////////////////
//
// Displays loudness data
/* ----------------------------------------------------------------------
													Object Structures
-------------------------------------------------------------------------
	loudness = float between 0 and 1, 0 meaning silences
*/
//////////////////////////////////////////////////////////////////////////
// Node.js Exports
var globalNamespace = {};
(function (exports) {
	exports.createNewLoudnessMeter = function( $, socket ) {
		newLoudnessMeter = new LoudnessMeter( $, socket );
		return newLoudnessMeter;
	};
}(typeof exports === 'object' && exports || globalNamespace));


//////////////////////////////////////////////////////////////////////////
// Constructor
function LoudnessMeter( $, socket ) {

	var _this = this;
	
	this.socket = socket;
	this.drawContext;
	
	var context = this.drawContext;

	if( typeof(exports) == "object" ) { 
		context = require("./CDrawContext").createNewDrawContext( $ );
	} else {
		context = new DrawContext( $ );
	}
	

	// Draw our background
	context.fillRect( 0, 0, context.getWidth(), context.getHeight(), "#fff" );
	
	// Draw our borders
	context.drawRect( 3.5, 3.5, context.getWidth()-6, context.getHeight()-6, "#000" );
	
	var self = this;
	this.socket.on( "newLoudnessData", function( data ) {
		self.drawLoudness( context, data.loudness );
	});
	
} // end LoudnessMeter()


//////////////////////////////////////////////////////////////////////////
// Updates the UI for the given loudness value
LoudnessMeter.prototype.drawLoudness = function( context, loudness ) {
	context.clear();
	
	// Draw our background
	context.fillRect( 0, 0, context.getWidth(), context.getHeight(), "#fff" );
	
	// Draw our borders
	context.drawRect( 3.5, 3.5, context.getWidth()-6, context.getHeight()-6, "#000" );
	
	// Draw our loudness
	var yLoudness = (loudness) * (context.getHeight() - 6);	
	context.fillRect( 3.5, context.getHeight() - 3.5 - yLoudness, context.getWidth()-6, yLoudness, "#6495ed" );
	
} // end LoudnessMeter.drawLoudness()

