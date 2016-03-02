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

    Nan::SetMethod(target, "createAudioEngine", Audio::AudioEngine::NewInstance);
    //NODE_SET_METHOD(target, "createAudioEngine", CreateEngine);
    //target->Set( NanNew<String>("createAudioEngine"), CreateEngine);
    //target->Set( NanNew<String>("createAudioEngine"), NanNew<FunctionTemplate>(CreateEngine)->GetFunction() );
    //target->Set( NanNew<String>("createAudioEngine"), NanNew<FunctionTemplate>(Audio::AudioEngine::NewInstance)->GetFunction() );
    
    target->Set( Nan::New<String>("sampleFormatFloat32").ToLocalChecked(), Nan::New<Number>(1));
    target->Set( Nan::New<String>("sampleFormatInt32").ToLocalChecked(), Nan::New<Number>(2) );
    target->Set( Nan::New<String>("sampleFormatInt24").ToLocalChecked(), Nan::New<Number>(4) );
    target->Set( Nan::New<String>("sampleFormatInt16").ToLocalChecked(), Nan::New<Number>(8) );
    target->Set( Nan::New<String>("sampleFormatInt8").ToLocalChecked(), Nan::New<Number>(10) );
    target->Set( Nan::New<String>("sampleFormatUInt8").ToLocalChecked(), Nan::New<Number>(20) );
}

NODE_MODULE( NodeCoreAudio, InitAll );
