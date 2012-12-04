// NodeCoreAudio.cpp : Defines the exported functions for the DLL application.
//

#include <v8.h>
using namespace v8;
//#pragma comment(lib, "node")
#include <node.h>

#include <string>
#include "AudioEngine.h"
#include <stdio.h>

Handle<Value> CreateEngine(const Arguments& args) {
    HandleScope scope;
    return scope.Close( Audio::AudioEngine::NewInstance(args) );
} // end CreateEngine()

void InitAll(Handle<Object> target) {
    Audio::AudioEngine::Init( target );

    target->Set( String::NewSymbol("createAudioEngine"), FunctionTemplate::New(CreateEngine)->GetFunction() );
}

NODE_MODULE( NodeCoreAudio, InitAll );
