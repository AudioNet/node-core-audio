// Outputs sine waves panning left & right while showing microphone input on screen

var coreaudio  = require('../../');
var qt = require('node-qt');


//Create a core audio engine
var engine = coreaudio.createNewAudioEngine();
engine.setOptions({ inputChannels: 1, outputChannels: 2, interleaved: true });

var sample = 0;
var ampBuffer = new Float32Array(4000);

engine.addAudioCallback(function(buffer) {
    var output = [];
    for (var i = 0; i < buffer.length; i++, sample++) {
        //Pan two sound-waves back and forth, opposing
        var val1 = Math.sin(sample * 110.0 * 2 * Math.PI / 44100.0) * 0.25, val2 = Math.sin(sample * 440.0 * 2 * Math.PI / 44100.0) * 0.25;
        var pan1 = Math.sin(1 * Math.PI * sample / 44100.0), pan2 = 1 - pan1;

        output.push(val1 * pan1 + val2 * pan2); //left channel
        output.push(val1 * pan2 + val2 * pan1); //right channel

        //Save microphone input into rolling buffer
        ampBuffer[sample%ampBuffer.length] = buffer[i];
    }
    return output;
});

//Create a qt window
var app = new qt.QApplication;
var window = new qt.QWidget;
global.app = app;
global.window = window;

var w = 640, h = 480;
var lastFPS = '', frames = 0, lastFrameTime = Date.now();

var backBrush = new qt.QColor(127, 127, 127);
var linePen = new qt.QPen(new qt.QColor(0, 0, 0));

//Every frame, draw the microphone data
window.paintEvent(function() {
    var p = new qt.QPainter();
    p.begin(window);
    p.fillRect(0, 0, w, h, backBrush);

    var path = new qt.QPainterPath();
    path.moveTo(new qt.QPointF(0, h / 2));
    for (var i = 0; i < ampBuffer.length; i++) {
        path.lineTo(new qt.QPointF(w * i / ampBuffer.length, h / 2 + ampBuffer[i] * h / 3.0));
    }
    p.strokePath(path, linePen);
    p.drawText(20, 30, 'FPS: ' + lastFPS, linePen);
    p.end();

    var newTime = Date.now();
    if (newTime - lastFrameTime > 1000) {
        lastFPS = frames;
        frames = 0;
        lastFrameTime = newTime;
    }

    frames++;
});

window.resize(w, h);
window.show();

setInterval(app.processEvents.bind(app), 0);
setInterval(function() { window.update(); }, 0); //Update our display as often as possible