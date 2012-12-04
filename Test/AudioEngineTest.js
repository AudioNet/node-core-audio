// Set this to something like 15000 to give yourself some time to attach to the node.exe process from a debugger
var TIME_TO_ATTACH_DEBUGGER_MS = 0;

setTimeout(function(){

    var util = require( "util" );

    process.on('uncaughtException', function (err) {
      console.error(err);
      console.log("Node NOT Exiting...");
    });

    var audioEngineImpl = require( "../build/Release/NodeCoreAudio" );

    console.log( audioEngineImpl );

    var buffer  = false;
    var output = [];

    var audioEngine = audioEngineImpl.createAudioEngine(
        {inputChannels: 2},
        function(input, lastInputOverflowed, lastOutputUnderflowed) {
            //output[0] = output[1] = input[0];
            return input; //just copy input to output, so output = input
        }
    );


    // Make sure the audio engine is still active
    if( audioEngine.isActive() ) console.log( "active" );
    else console.log( "not active" );

    var c = 0;
    setInterval(function(){
        if( audioEngine.isActive() ) c++;

    }, 1000 );

    setTimeout(function(){
        audioEngine.setOptions({
            framesPerBuffer: 5000
        });
    }, 5000);

}, TIME_TO_ATTACH_DEBUGGER_MS);