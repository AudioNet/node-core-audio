//////////////////////////////////////////////////////////////////////////////
//
// NodeCoreAudio.cpp : Main module source, declares all exports
// 
//////////////////////////////////////////////////////////////////////////////

#include <v8.h>
using namespace v8;
//#pragma comment(lib, "node")
#include <node.h>

#include <string>
#include "AudioEngine.h"
#include <stdio.h>

/*
//Handle<Value> CreateEngine(const Arguments& args) 
NAN_METHOD(CreateEngine)
{
    //HandleScope scope;
    NanScope();
    //return scope.Close( Audio::AudioEngine::NewInstance(args) );
    NanReturnValue(Audio::AudioEngine::NewInstance(args));
} // end CreateEngine()
*/

void InitAll(Handle<Object> target) {
    Audio::AudioEngine::Init( target );

    NODE_SET_METHOD(target, "createAudioEngine", Audio::AudioEngine::NewInstance);
    //NODE_SET_METHOD(target, "createAudioEngine", CreateEngine);
    //target->Set( NanNew<String>("createAudioEngine"), CreateEngine);
    //target->Set( NanNew<String>("createAudioEngine"), NanNew<FunctionTemplate>(CreateEngine)->GetFunction() );
    //target->Set( NanNew<String>("createAudioEngine"), NanNew<FunctionTemplate>(Audio::AudioEngine::NewInstance)->GetFunction() );
    
    target->Set( NanNew<String>("sampleFormatFloat32"), NanNew<Number>(1) );
    target->Set( NanNew<String>("sampleFormatInt32"), NanNew<Number>(2) );
    target->Set( NanNew<String>("sampleFormatInt24"), NanNew<Number>(4) );
    target->Set( NanNew<String>("sampleFormatInt16"), NanNew<Number>(8) );
    target->Set( NanNew<String>("sampleFormatInt8"), NanNew<Number>(10) );
    target->Set( NanNew<String>("sampleFormatUInt8"), NanNew<Number>(20) );
}

NODE_MODULE( NodeCoreAudio, InitAll );
