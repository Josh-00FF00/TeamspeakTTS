#include "public_definitions.hpp"
#include "ts3_functions.hpp"
#include "plugin.hpp"
#include "TTS.hpp"
#include <Windows.h>
#include <string>
#include <queue>
#include <map>
#include <list>
#include <functional>
#include <thread>
#include <sapi.h>

using namespace std;
#define FALSE 0
#define TRUE 1


inline const char * const BoolToString(bool b)
{
	return b ? "true" : "false";
}

TTS::TTS() :
thread_running(false), 
accepted_chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789!?\"£$%&*#'@;:/\\()[].,-=+<>") {

	/* Maps the teamspeak commands to the TTS functions that match them. arguments are expected
	 * as a std::list of strings
	*/ 
	toggles = {
		{ "mute", false },
	    { "talkback", false },
	};
	using namespace std::placeholders; // for _1, _2, _3...
	commands = { // placehoders are the list of string arguments passed
		{"toggle", std::bind(&TTS::toggle, this, _1)},
		{"set", std::bind(&TTS::set, this, _1) }
	};

	values = {
		{"maxlength", 5000},
		{"maxqueue", 5}
	};
}

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

void TTS::setFunctionPointers(TS3Functions funcs) {
	ts3funcs = funcs;
}

int TTS::shutdown(){
	ULONG skipped;
	std::future_status thread_status;
	int maxlength = get_maxlength();
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


int TTS::toggle(std::list<std::string> arguments) {
	string msg;
	if (arguments.size() != 1) {
		ts3funcs.printMessageToCurrentTab("Toggle requires 1 argument!");
		return 0;
	}

	if (toggles.count(arguments.front()) > 0) { // if the toggle exists in the map
		toggles[arguments.front()] = !toggles[arguments.front()];
		msg = "Toggled: ";
		msg.append(arguments.front());
		msg.append(" to ");
		msg.append(BoolToString(toggles[arguments.front()]));
	}
	else {
		msg = "Cannot find toggle: ";
		msg.append(arguments.front());
	}

	ts3funcs.printMessageToCurrentTab(msg.c_str());

	return 0;
}

int TTS::set(std::list<std::string> arguments) {
	if (arguments.size() != 2) {
		ts3funcs.printMessageToCurrentTab("Set requires 2 arguments!");
		return 0;
	}

	string fstArg = arguments.front();
	string sndArg = *next(arguments.begin());

	if (values.count(fstArg) > 0) { // if the value exists in the map
		string msg;
		try {
			int value = stoi(sndArg); //gets the second argument and converts to int
			values[fstArg] = value;

			msg = "Set: ";
			msg.append(fstArg);
			msg.append(" to ");
			msg.append(sndArg);
			ts3funcs.printMessageToCurrentTab(msg.c_str());
		}
		catch (const std::invalid_argument) {
			msg = "Cannot convert: ";
			msg.append(sndArg);
			ts3funcs.printMessageToCurrentTab(msg.c_str());
		}
	}
	else {
		string msg = "Cannot find value: ";
		ts3funcs.printMessageToCurrentTab(msg.append(fstArg).c_str());
	}

	return 0;
}

std::string::size_type TTS::pushmessage(string message){
	/*Wrapper for queue, checks against max size and discards oldest messages until queue.size() == maxqueue.
	RETURNS new queue size*/

	m_queue.push(message);
	unsigned int maxqueue = get_maxqueue();
	if (m_queue.size() > maxqueue){
		accessing.lock();
		for (unsigned int i = 0; i < (m_queue.size() - maxqueue); i++){
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
		unsigned int maxqueue = get_maxqueue();
		int maxlength = get_maxlength();
		for (unsigned int i = 0; i < m_queue.size(); i++){

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