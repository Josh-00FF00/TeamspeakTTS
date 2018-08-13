#ifndef GUARD_TTS
#define GUARD_TTS

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <sapi.h>
#include <map>
#include <list>
#include "ts3_functions.hpp"

#define TIMEOUT 5000 //time to wait for thread to end in milliseconds
using namespace std;

class TTS{
public:
	TTS();
	int initialise();
	void setFunctionPointers(TS3Functions funcs);
	int shutdown();

	int speak_thread();
	std::string::size_type pushmessage(std::string message);

	int toggle(std::list<std::string>);
	int set(std::list<std::string>);

	int get_maxqueue() { return values["maxqueue"]; };
	bool get_talkback() { return toggles["talkback"]; };
	bool get_mute() { return toggles["mute"]; };
	int get_maxlength() { return values["maxlength"]; };

	std::string accepted_chars;

	std::map<std::string, std::function<int(std::list<std::string>)>> commands;


private:

	bool thread_running;

	TS3Functions ts3funcs;

	std::queue<std::string> m_queue;
	map<string, bool> toggles;
	map<string, unsigned int> values;


	std::mutex empty;
	std::mutex accessing;
	std::condition_variable not_empty;
	std::future<int> thread_ended;

	ISpVoice * pVoice = NULL;
	HRESULT hr = NULL;

	wchar_t* widechar_convert(std::string message);
};

#endif