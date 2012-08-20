//////////////////////////////////////////////////////////////////////////
// Node.js Exports
var globalNamespace = {};
(function (exports) {
	exports.createNewDrawContext = function( $, elementName ) {
		newDrawContext= new DrawContext( $, elementName );
		return newDrawContext;
	};
}(typeof exports === 'object' && exports || globalNamespace));

//////////////////////////////////////////////////////////////////////////
// Constructor
function DrawContext( $, elementName ) {
	this.elementName = "canvas";
	this.canvas;
	this.context;

	// Default our element name
	if( typeof elementName != "undefined" ) {
		this.elementName = elementName;
	}
	
	// Find the canvas element.
    this.canvas = $( this.elementName ).get( 0 );
	
	// Get the 2D canvas context.
    this.context = this.canvas.getContext('2d');
} // end DrawContext.DrawContext()


//////////////////////////////////////////////////////////////////////////
// Returns the height of the canvas
DrawContext.prototype.setAlpha = function( alpha ) {
	return this.context.globalAlpha = alpha;
} // end UserSession.getHeight()


//////////////////////////////////////////////////////////////////////////
// Returns the height of the canvas
DrawContext.prototype.getHeight = function() {
	return this.canvas.height;
} // end UserSession.getHeight()


//////////////////////////////////////////////////////////////////////////
// Returns the width of the canvas
DrawContext.prototype.getWidth = function() {
	return this.canvas.width;
} // end UserSession.getWidth()


//////////////////////////////////////////////////////////////////////////
// Returns the width of the canvas
DrawContext.prototype.clearRect = function( xLoc, yLoc, width, height ) {
	this.context.clearRect( xLoc, yLoc, width, height );
} // end UserSession.clearRect()


//////////////////////////////////////////////////////////////////////////
// Returns the width of the canvas
DrawContext.prototype.clear = function() {
	this.clearRect( 0, 0, this.getWidth(), this.getHeight() );
} // end UserSession.clearRect()


//////////////////////////////////////////////////////////////////////////
// Draw a line
DrawContext.prototype.drawLine = function( xStart, yStart, xEnd, yEnd, lineWidth, color ) {
	// Default our line width
	if( typeof lineWidth == "undefined" ) {
		lineWidth = 1;
	}
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}
	
	// Set our color
	this.context.strokeStyle = color;
	
	// Draw the line
	this.context.lineWidth = lineWidth;
	this.context.beginPath();
	this.context.moveTo( xStart, yStart );
	this.context.lineTo( xEnd, yEnd );
	this.context.stroke();
} // end UserSession.drawLine()


//////////////////////////////////////////////////////////////////////////
// Draw a polygon using X and Y coordinates
DrawContext.prototype.drawPolyXY = function( fX, fY, lineWidth, color ) {
	// Default our line width
	if( typeof lineWidth == "undefined" ) {
		lineWidth = 1;
	}
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}
		
	// Jump out unless we have more than two locations to draw
	if( fX.length < 2 || fY.length < 2 ) { return; }
	
	// Set our color
	this.context.strokeStyle = color;
	
	// Begin the line
	this.context.lineWidth = lineWidth;
	this.context.beginPath();
		
	// Move the context to the first element in our x/y functions
	this.context.moveTo( fX[0], fY[0] );
		
	// Connect that first point to every other point in the arrays
	for( iPoint=1; iPoint<fX.length; ++iPoint ) {
		this.context.lineTo( fX[iPoint], fY[iPoint] );
	}

	// Actually draw the polygon
	this.context.stroke();
} // end UserSession.drawPoly()


//////////////////////////////////////////////////////////////////////////
// Draw a polygon using Point structs
DrawContext.prototype.drawPolyPoint = function( path, lineWidth, color ) {
	// Default our line width
	if( typeof lineWidth == "undefined" ) {
		lineWidth = 1;
	}
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}
		
	// Jump out unless we have more than two locations to draw
	if( path.length < 2 ) { return; }
	
	// Set our color
	this.context.strokeStyle = color;
	
	// Begin the line
	this.context.lineWidth = lineWidth;
	this.context.lineJoin = "bevel";
	this.context.beginPath();
		
	// Move the context to the first element in our x/y functions
	this.context.moveTo( path[0].x, path[0].y );
		
	// Connect that first point to every other point in the arrays
	for( iPoint=1; iPoint<path.length; ++iPoint ) {
		this.context.lineTo( path[iPoint].x, path[iPoint].y );
	}
	
	this.context.stroke();
} // end UserSession.drawPolyPoint()


//////////////////////////////////////////////////////////////////////////
// Draw a polygon and fills it in
DrawContext.prototype.fillPoly = function( fX, fY, color ) {
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}
		
	// Jump out unless we have more than two locations to draw
	if( fX.length < 2 || fY.length < 2 ) { return; }
	
	// Set our color
	this.context.strokeStyle = color;
	this.context.fillStyle = color;
	
	// Begin the line
	this.context.lineWidth = lineWidth;
	this.context.beginPath();
		
	// Move the context to the first element in our x/y functions
	this.context.moveTo( fX[0], fY[0] );
		
	// Now connect that first point to every other point in the arrays
	for( iPoint=1; iPoint<fX.length; ++iPoint ) {
		this.context.lineTo( fX[iPoint], fY[iPoint] );
	}
	
	// Fill the polygon and stroke around it
	this.context.fill();
	this.context.stroke();
} // end UserSession.fillPoly()


//////////////////////////////////////////////////////////////////////////
// Draw an array of paths using relative coordinates
DrawContext.prototype.drawMultiPolyPointRelative = function( paths, lineWidth, color ) {
	// Default our line width
	if( typeof lineWidth == "undefined" ) {
		lineWidth = 1;
	}
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}

	// Set our line settings
	this.context.strokeStyle = color;	
	this.context.lineWidth = lineWidth;
	
	// Relative points come in as % width and height
	// We need to loop through and determine our pixel values
	for( iPath=0; iPath<paths.length; ++iPath ) {
		// Grab the path we're looking at
		var currentPath = paths[iPath];
		
		// Get our absolute pixel values
		currentPath = this.mapPathFromRelative( currentPath );
		
		// Draw the path
		this.drawPolyPoint( currentPath, lineWidth, color );
	}
} // end UserSession.drawMultiPolyPointRelative()


//////////////////////////////////////////////////////////////////////////
// Returns the path scaled to pixel values from % width and height
DrawContext.prototype.mapPathFromRelative = function( path ) {
	var absolutePath = new Array();

	// Find the appropriate pixel values
	for( iPoint=0; iPoint<path.length; ++iPoint ) {
		absolutePoint = {
			x: Math.round( path[iPoint].x * this.getWidth() ) + 0.5,
			y: Math.round( path[iPoint].y * this.getHeight() ) + 0.5
		};
		
		absolutePath.push( absolutePoint );
	}
	
	return absolutePath;
} // end UserSession.mapPathToRelative()


//////////////////////////////////////////////////////////////////////////
// Returns the path scaled to % width and height of this drawContext
DrawContext.prototype.mapPathToRelative = function( path ) {
	var relativePath = new Array();

	for( iPoint=0; iPoint<path.length; ++iPoint ) {
		relativePoint = {
			x: path[iPoint].x / this.getWidth(),
			y: path[iPoint].y / this.getHeight()
		};
		
		relativePath.push( relativePoint );
	}
	
	return relativePath;
} // end UserSession.mapPathToRelative()


//////////////////////////////////////////////////////////////////////////
// Draw a rectangle
DrawContext.prototype.drawRect = function( xLoc, yLoc, width, height, lineWidth, color ) {
	// Default our line width
	if( typeof lineWidth == "undefined" ) {
		lineWidth = 1;
	}
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}

	// Set our color
	this.context.strokeStyle = color;
	
	this.context.lineWidth = lineWidth;
	this.context.strokeRect( xLoc, yLoc, width, height );
} // end UserSession.drawRect()


//////////////////////////////////////////////////////////////////////////
// Draw a rectangle
DrawContext.prototype.fillRect = function( xLoc, yLoc, width, height, color ) {
	
	// Default our color
	if( typeof color == "undefined" ) {
		color = '#000';
	}

	// Set our color
	this.context.fillStyle = color;
	this.context.strokeStyle = color;
	
	this.context.fillRect( xLoc, yLoc, width, height );
} // end UserSession.fillRect()
