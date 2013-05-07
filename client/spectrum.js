//////////////////////////////////////////////////////////////////////////
// Spectrum - Client Side
//////////////////////////////////////////////////////////////////////////
//
// Draws a spectrum of incoming FFT data
// 
/* ----------------------------------------------------------------------
                                                    Object Structures
-------------------------------------------------------------------------

*/
//////////////////////////////////////////////////////////////////////////
// Namespace (lol)
var BORDER_WIDTH = 30,
	MAX_VALUE = 200;


//////////////////////////////////////////////////////////////////////////
// Constructor
function Spectrum( sampleRate ) {
	var _this = this;

	this.sampleRate = sampleRate || 44100;
	this.canvas = document.getElementById( "spectrumCanvas" );
	this.context = this.canvas.getContext("2d");

	this.toolTip = new ToolTip( "a", false );

	// Setup our tooltip
	this.canvas.onmousemove = function(event) {
		var parentOffset = $("#spectrumCanvas").parent().offset(); 

		var xChunk = (_this.canvas.width - BORDER_WIDTH) / 24,
			yChunk = (_this.canvas.height - BORDER_WIDTH) / 7;

        var xLoc = event.pageX - parentOffset.left,
        	yLoc = event.pageY - parentOffset.top + BORDER_WIDTH/2;

    	var iDay = Math.floor( (yLoc - BORDER_WIDTH) / yChunk ),
    		iHour = Math.floor( (xLoc - BORDER_WIDTH) / xChunk );

    	_this.toolTip.Show( event, "_this.histogram[iDay][iHour]" );
	}

	this.canvas.onmouseout = function(event) {
		_this.toolTip.Hide( event );
	}

} // end Spectrum()


//////////////////////////////////////////////////////////////////////////
// Main drawing function
Spectrum.prototype.draw = function( magnitudes ) {

	var context = this.context,
		canvas = this.canvas;

	// Clear what was there
	this.clear();

	this.drawBorder();

	context.fillStyle = '#f00';
	context.beginPath();
	context.moveTo( 0, this.canvas.height );

	context.lineTo( this.freqToX(0), this.canvas.height );

	for( var iCoeff=0; iCoeff<magnitudes.length; ++iCoeff ) {

		var magnitude = magnitudes[iCoeff],
			freq = this.coeffIndexToFreq( iCoeff, magnitudes.length );

		context.lineTo( this.freqToX(freq), this.magToY(magnitude) );
	}

	context.lineTo( canvas.width, canvas.height );
	
	context.closePath();
	context.fill();

	for( var iCoeff=0; iCoeff<magnitudes.length; ++iCoeff ) {

		var magnitude = magnitudes[iCoeff],
			freq = this.coeffIndexToFreq( iCoeff, magnitudes.length );

		var xLoc = this.freqToX(freq);
		
		context.beginPath();
		context.moveTo( xLoc, this.magToY(magnitude) );
		context.lineTo( xLoc, canvas.height-1 );
		context.stroke();
		context.closePath();
	}

} // end Spectrum.draw()


//////////////////////////////////////////////////////////////////////////
// Draw our borders
Spectrum.prototype.drawBorder = function( iCoeff, numCoeffs ) {
	var context = this.context,
		canvas = this.canvas;

	context.beginPath();
	context.moveTo( 0, 0 );
	context.lineTo( 0, canvas.height-1 );
	context.lineTo( canvas.width-1, canvas.height-1 );
	context.lineTo( 0, canvas.height-1 );
	context.moveTo( 0, 0 );
	context.stroke();
	context.closePath();
} // end Spectrum.drawBorder()


//////////////////////////////////////////////////////////////////////////
// Main drawing function
Spectrum.prototype.coeffIndexToFreq = function( iCoeff, numCoeffs ) {
	var nyquist = this.sampleRate * 0.5;
	return nyquist * (iCoeff / numCoeffs);
} // end Spectrum.coeffIndexToFreq()



//////////////////////////////////////////////////////////////////////////
// Main drawing function
Spectrum.prototype.freqToX = function( freq ) {
	var nyquist = this.sampleRate * 0.5,
		xMax = Math.log( nyquist );

	return this.canvas.width * Math.log(freq) / xMax;
} // end Spectrum.freqToX()


//////////////////////////////////////////////////////////////////////////
// Main drawing function
Spectrum.prototype.magToY = function( mag ) {
	var dB = 20*Math.log( Math.abs(mag) );

	var yVal = this.canvas.height - this.canvas.height * (dB / MAX_VALUE);
	
	return yVal > this.canvas.height ? this.canvas.height : yVal;
} // end Spectrum.draw()


//////////////////////////////////////////////////////////////////////////
// Clears the canvas
Spectrum.prototype.clear = function() {
	var context = this.context,
		canvas = this.canvas;

	// Store the current transformation matrix
	context.save();

	// Use the identity matrix while clearing the canvas
	context.setTransform(1, 0, 0, 1, 0, 0);
	context.clearRect(0, 0, canvas.width, canvas.height);

	// Restore the transform
	context.restore();

	// Reset our max histogram value
	this.maxHistogramValue = 0;
} // end Spectrum.clear()