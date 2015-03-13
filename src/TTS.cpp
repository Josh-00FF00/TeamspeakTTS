#include "public_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include "TTS.h"
#include <Windows.h>
#include <string>
#include <queue>
#include <thread>
#include <sapi.h>

using namespace std;
#define FALSE 0
#define TRUE 1

TTS::TTS() : mute(FALSE), 
talkback(FALSE), 
maxlength(100), 
maxqueue(5), 
thread_running(false), 
accepted_chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789!?\"£$%&*#'@;:/\\()[]") {}

int TTS::initialise(){

	if (FAILED(CoInitialize(NULL)))
		return 1;

	hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
	if (!SUCCEEDED(hr))
		return 1;

	thread_running = true;
	std::packaged_task<int()> task([this]() -> int {return TTS::speak_thread();});  //Fancy new packaged future stuff here to replace the old method of checking whether my thread has ended by  bool and hope no races happen, somewhat less of a hack.
	thread_ended = task.get_future();
	thread t1(std::move(task));
	
	t1.detach();

	return 0;

}

int TTS::shutdown(){
	ULONG skipped;
	std::future_status thread_status;

	pVoice->Skip(L"SENTENCE", maxlength, &skipped);	//Skip maxlength sentences, this should end the tts speaking
	pVoice->WaitUntilDone(TIMEOUT);
	
	thread_running = false;							//start to shutdown thread
	not_empty.notify_one();

	thread_status = thread_ended.wait_for(std::chrono::milliseconds(TIMEOUT));	//wait for thread to end or until TIMEOUT, whichever is first

	pVoice->Release();
	pVoice = NULL;

	CoUninitialize();

	if (thread_status != std::future_status::ready){
		return 1;										//This only happens when the thread hasn't exited properly, NOTE: Add actual error to throw
	}
	else{
		return thread_ended.get();
	}
}

string TTS::set_maxlength(int newlength){
	
	if (newlength > 0 && newlength < (int) TS3_MAX_SIZE_TEXTMESSAGE){
		maxlength = newlength;
	}else{
		throw "Out of bound max length - must be between 0 and " + to_string(TS3_MAX_SIZE_TEXTMESSAGE);
	}
	return "Max Length = " + to_string(maxlength);

}

string TTS::set_maxqueue(int newqueue){
	maxqueue = newqueue;
	
	return "New queue size = " + to_string(maxqueue);
}

string TTS::toggle_mute(){

	mute = !mute;
	
	if (mute){
		return "Mute is ENABLED";
	}else{
		return "Mute is DISABLED";
	}
}

string TTS::toggle_talkback(){
	talkback = !talkback;

	if (talkback){
		return "Talkback is ENABLED";
	}
	else{
		return "Talkback is DISABLED";
	}

}

std::string::size_type TTS::pushmessage(string message){
	/*Wrapper for queue, checks against max size and discards oldest messages until queue.size() == maxqueue.
	RETURNS new queue size*/

	m_queue.push(message);
	
	if (m_queue.size() > maxqueue){
		accessing.lock();
		for (int i = 0; i < (m_queue.size() - maxqueue); i++){
			m_queue.pop();
		}
		accessing.unlock();
	}

	not_empty.notify_one();

	return m_queue.size();
}

wchar_t* TTS::widechar_convert(string message){
	int wchars_num = MultiByteToWideChar(CP_ACP, 0, message.c_str(), -1, NULL, 0);

	wchar_t* u_message = new wchar_t[wchars_num];
	ZeroMemory(u_message, wchars_num);

	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, message.c_str(), -1, u_message, wchars_num);

	return u_message;
	
}

int TTS::speak_thread(){
	std::unique_lock<std::mutex> empty_lk(empty);
	wchar_t* speech;
	
	while (thread_running){
		
		not_empty.wait(empty_lk, [this](){ return m_queue.size() > 0 || !thread_running; });

		for (int i = 0; i < m_queue.size(); i++){

			if (i >= maxqueue-1){
				break;
			}

			speech = widechar_convert((m_queue.front()).substr(0, maxlength));
			hr = pVoice->Speak(speech, SPF_DEFAULT, NULL);
			free(speech);

			accessing.lock();
			m_queue.pop();
			accessing.unlock();

			if (!thread_running){
				break;
			}
		}

	}
	return 0;
}