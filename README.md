# TeamspeakTTS

This is a _very_ simple TTS engine I've added for windows only. It should probably work on all Windows versions since Vista. This reads typed chat messages from the room you're in, then speaks them at you so you don't even need to read them!

## Features
* Can't get enough of your teammates voices and want to hear their awful text messages as well? Now you can.
* Ability to mute this ^ (Quickly added after user testing resulted in a _bad_ experience).
* Grabs the domain name of posted URLs so you're not blasted with a 1000 character long url that someone badly copied into chat.

## Commands
Currently there are a few options that are accessed by typing "/tts" into chat.

* /tts toggle mute (mutes tts from chat)
* /tts toggle talkback (allows you to hear your own chat messages)
----

* /tts set maxlength 100 (Sets the maxlength of a message that will be read)
* /tts set maxqueue 5 (Sets the number of messages saved to be read out, oldest removed first)

That's it!

## Installation

### Manual

Place the correct dll in your Teamspeak plugins directory, and then go into "Tools -> Options -> Addons" and enable "Simple TTS Plugin".

The Teamspeak plugin directory is usually located at:

```
C:\Users\<USERNAME HERE>\AppData\Roaming\TS3Client\plugins
```

[64 bit windows](https://github.com/JoshGreen0x04/TeamspeakTTS/raw/master/build/Release/TeamspeakTTS_x64.dll)
[32 bit windows](https://github.com/JoshGreen0x04/TeamspeakTTS/raw/master/build/Release/TeamspeakTTS_x86.dll)

Now your chat messages will be read out.

### Automatic

TODO.
