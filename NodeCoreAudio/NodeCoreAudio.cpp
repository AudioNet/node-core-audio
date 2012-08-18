// NodeCoreAudio.cpp : Defines the exported functions for the DLL application.
//
#define BUILDING_NODE_EXTENSION
#include "stdafx.h"

#include <v8.h>
using namespace v8;
#pragma comment(lib, "node")
#include <node.h>

#include <string>
#include "AudioEngine.h"

#include <WinSock2.h>
#include <IPHlpApi.h>
#include <stdio.h>
#include <Windows.h>
#pragma comment(lib, "IPHLPAPI.lib")


#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


struct Baton {
	uv_work_t request;
	Persistent<Function> callback;
	int error_code;
	std::string error_message;

	// Custom data
	int32_t result;
};


void CallCallback(uv_work_t* req) {
	HandleScope scope;
	Baton* baton = static_cast<Baton*>(req->data);

	baton->callback.Dispose();
	delete baton;
} // end CallCallback


namespace CoreAudio {
	Handle<Value> InitializeAudioEngine(const Arguments &args) {

		HandleScope scope;
		return scope.Close(Audio::AudioEngine::NewInstance(args));

// 		HandleScope scope;
// 		
// 		if (!args[0]->IsFunction()) {
// 			return ThrowException( Exception::TypeError(String::New("Callback function required")) );
// 		}
// 
// 		Local<Function> callback = Local<Function>::Cast(args[0]);
// 
// 		Baton* baton = new Baton();
// 		baton->request.data = baton;
// 		baton->callback = Persistent<Function>::New(callback);
// 
// 		//uv_queue_work(uv_default_loop(), &baton->request, AsyncWork, AsyncAfter);
// 
// 		Audio::AudioEngine* pAudioEngine= new Audio::AudioEngine( baton->callback );
// 
// 		return scope.Close(Undefined());

	} // end InitializeAudioEngine()
}

// void AsyncWork(uv_work_t* req) {
// 	// No HandleScope!
// 
// 	Baton* baton = static_cast<Baton*>(req->data);
// 
// 	// Do work in threadpool here.
// 	// Set baton->error_code/message on failures.
// }

// extern "C" void NODE_EXTERN init (Handle<Object> target)
// {
// 	HandleScope scope;
// 	Local<FunctionTemplate> initializeAudioEngine = FunctionTemplate::New( NodeCoreAudio::InitializeAudioEngine );
// 	
// 	target->Set( String::NewSymbol("initializeAudioEngine"), initializeAudioEngine->GetFunction() );
// }

Handle<Value> CreateEngine(const Arguments& args) {
	HandleScope scope;
	return scope.Close(Audio::AudioEngine::NewInstance(args));
}

void InitAll(Handle<Object> target) {
	Audio::AudioEngine::Init( target );

	target->Set(String::NewSymbol("createAudioEngine"),
		FunctionTemplate::New(CreateEngine)->GetFunction());
}

NODE_MODULE( NodeCoreAudio, InitAll )
