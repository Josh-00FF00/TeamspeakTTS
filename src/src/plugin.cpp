/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2013 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

#include <sapi.h>
#include <string>
#include <vector>

#include <iostream>

using namespace std;

static struct TS3Functions ts3Functions;
static struct TTS tts_options;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 19

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128
#define MESSAGE_BUFSIZE 1024

static char* pluginID = NULL;
ISpVoice * pVoice = NULL;
HRESULT hr = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

int speak(const char* message){
		
		int wchars_num =  MultiByteToWideChar( CP_ACP , 0 , message , -1, NULL , 0 );
	
		wchar_t* u_message = new wchar_t[wchars_num];
		ZeroMemory(u_message, wchars_num);
	
		MultiByteToWideChar(CP_ACP , MB_COMPOSITE , message , -1 , u_message , wchars_num);

		hr = pVoice->Speak(u_message, SPF_ASYNC, NULL);

		return 0;
}

std::vector<url_cut> url_extract(const string& message){
	vector<url_cut> ret;

	string::size_type start, end;
	url_cut entry;
	
	start = message.find("[URL]");
	end = message.find("[/URL]");

	while(start != string::npos && end != string::npos){
		entry.start = start;
		entry.end = end + 6; //+6 to get to the end of the /URL segment
		entry.url = message.substr(entry.start, entry.end-entry.start);

		ret.push_back(entry);
		
		start = message.find("[URL]", start+1); //+1 so the find pos searches at that position and after
		end = message.find("[/URL]", end+1);

	}
	return ret;
}

string return_domain(const string& long_url){
	string ret;
	string::size_type start, end;	
	
	start = long_url.find("://") + strlen("://"); //find the first character of domain
	end = long_url.find("/", start); //from start so end isn't the first slash in identifier

	if(end == string::npos){
		end = long_url.length();
	}
	
	if (start != string::npos && end != string::npos){
		ret = long_url.substr(start, end-start);
		return ret;
	}
	ret = "";
	return ret;
}

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */

	return "Simple TTS Plugin";
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Joshua Green";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Simple TTS plugin for chat messages in Teamspeak";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    if (FAILED(CoInitialize(NULL)))
        return 1;

    hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if(!SUCCEEDED(hr))
		return 1;

	tts_options.mute = 0;			//setup defaults in the teamspeak strut
	tts_options.talkback = 0;
	tts_options.maxlength = 100;


    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	pVoice->Release();
    pVoice = NULL;

    CoUninitialize();
	
	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "tts";
}


/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {
	
	anyID myId;
	vector<url_cut> url_index;
	string str_message(message);
	string speech;
	string cut_url;
	int skip=0;

	ts3Functions.getClientID(serverConnectionHandlerID, &myId);

	if((fromID == myId) && !tts_options.talkback){ //if talkback is disabled don't repeat own message
		return 0;
	}
	if(!tts_options.mute){							//Only speak if unmuted
		url_index = url_extract(str_message);			//check if contains a URL, returns an empy vector if not found or the indexes of the URL

		for(int i = 0; i < url_index.size(); i++){
			
			cut_url = "link from domain " + return_domain(url_index[i].url);
			speech.append(message+skip, url_index[i].start-skip);
			speech += cut_url;
			skip = url_index[i].end;
		}
		speech.append(message+skip);
		speak(speech.substr(0, tts_options.maxlength).c_str());
	}

    return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command){
	char buf[COMMAND_BUFSIZE];
	char *s, *param1 = NULL, *param2 = NULL;
	int i = 0;
	enum { CMD_NONE = 0, CMD_TALKBACK, CMD_MUTE, CMD_PITCH, CMD_SPEED, CMD_MAXLENGTH, CMD_STOP} cmd = CMD_NONE;
#ifdef _WIN32
	char* context = NULL;
#endif

	_strcpy(buf, COMMAND_BUFSIZE, command);
#ifdef _WIN32
	s = strtok_s(buf, " ", &context);
#else
	s = strtok(buf, " ");
#endif
	while(s != NULL) {
		if(i == 0) {
			if(!strcmp(s, "talkback")) {			//used for client to read own message
				cmd = CMD_TALKBACK;
			} else if(!strcmp(s, "mute")) {			//used to mute all output
				cmd = CMD_MUTE;
			} else if(!strcmp(s, "pitch")) {		//Unimplemented
				cmd = CMD_PITCH;
			} else if(!strcmp(s, "speed")) {		//Unimplemented
				cmd = CMD_SPEED;
			}  else if(!strcmp(s, "maxlength")){	//Used to set the maximun number of characters to read from the message
				cmd = CMD_MAXLENGTH;
			}	else if(!strcmp(s, "stop")){		//Unimplemented
				cmd = CMD_STOP;
			}
		} else if(i == 1) {
			param1 = s;
		} else {
			param2 = s;
		}
#ifdef _WIN32
		s = strtok_s(NULL, " ", &context);
#else
		s = strtok(NULL, " ");
#endif
		i++;
	}

	switch(cmd) {
		case CMD_TALKBACK: {
			tts_options.talkback = !tts_options.talkback;
			if (tts_options.talkback)
				ts3Functions.printMessageToCurrentTab("Talkback is ENABLED");
			else
				ts3Functions.printMessageToCurrentTab("Talkback is DISABLED");
			break;
						   }
		case CMD_MUTE:{
			tts_options.mute = !tts_options.mute;
			if (tts_options.mute)
				ts3Functions.printMessageToCurrentTab("Mute is ENABLED");
			else
				ts3Functions.printMessageToCurrentTab("Mute is DISABLED");
			break;
					  }
		
		case CMD_MAXLENGTH:{
			tts_options.maxlength = atoi(param1);

			string out = "Maxlength is: ";
			out += param1;

			ts3Functions.printMessageToCurrentTab(out.c_str());
			break;
						   }
		case CMD_STOP:{
			break;
						   }
	
	}
	return 0;
	}

