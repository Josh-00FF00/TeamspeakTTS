#ifndef GUARD_TTS
#define GUARD_TTS

#include<string>
#include<queue>
#include<thread>
#include<mutex>
#include<future>
#include <condition_variable>
#include <sapi.h>

#define TIMEOUT 5000 //time to wait for thread to end in milliseconds

class TTS{
public:
	TTS();
	int initialise();
	int shutdown();

	int speak_thread();
	std::string::size_type pushmessage(std::string message);

	std::string set_maxlength(int newlength);
	int get_maxlength() const { return maxlength; };

	std::string set_maxqueue(int newqueue);
	int get_maxqueue() const { return maxqueue; };

	std::string toggle_talkback();
	bool get_talkback() const { return talkback; };

	std::string toggle_mute();
	bool get_mute() const { return mute; };

	std::string accepted_chars;


private:
	bool talkback;
	bool mute;

	bool thread_running;

	int maxlength;
	int maxqueue;
	std::queue<std::string> m_queue;

	std::mutex empty;
	std::mutex accessing;
	std::condition_variable not_empty;
	std::future<int> thread_ended;

	ISpVoice * pVoice = NULL;
	HRESULT hr = NULL;

	wchar_t* widechar_convert(std::string message);
};

#endif