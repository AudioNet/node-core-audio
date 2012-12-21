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
    
    target->Set( String::NewSymbol("sampleFormatFloat32"), Number::New(1) );
    target->Set( String::NewSymbol("sampleFormatInt32"), Number::New(2) );
    target->Set( String::NewSymbol("sampleFormatInt24"), Number::New(4) );
    target->Set( String::NewSymbol("sampleFormatInt16"), Number::New(8) );
    target->Set( String::NewSymbol("sampleFormatInt8"), Number::New(10) );
    target->Set( String::NewSymbol("sampleFormatUInt8"), Number::New(20) );
}

NODE_MODULE( NodeCoreAudio, InitAll );
