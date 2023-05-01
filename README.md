# ofxWhisper

`ofxWhisper` is an openFrameworks addon for transcribing recorded audio or audio files in real-time. This addon sends audio data and uses the OpenAI Whisper ASR API to convert the audio data into text.

## Features

- Transcribe audio file
- Real-time transcription of mic input
- Thread-safe implementation

## Usage

### Setup

1. Copy the `ofxWhisper` folder to the `of_v0.X.X_addons/` directory.
2. Add the following dependencies to your project:

- [ofxPoco](https://openframeworks.cc/documentation/ofxPoco/) (oF default addon)
- [arturoc/ofxHttpUtils](https://github.com/arturoc/ofxHttpUtils)
- [npisanti/ofxAudioFile](https://github.com/npisanti/ofxAudioFile)
- [roymacdonald/ofxSoundObjects](https://github.com/roymacdonald/ofxSoundObjects)

### Create and initialize an instance

```cpp
#include "ofxWhisper.h"

ofxWhisper whisper;

void ofApp::setup() {
    whisper.setup("your_api_key_here");
    whisper.setupRecorder(); // Optionally pass a sound device ID
}
```

### Transcribe from file

```    
whisper.transcript("sample_speech.mp3");

```

```
void ofApp::update() {
    // Check if the transcript is ready
    if (whisper.hasTranscript()) {
        transcript = whisper.getNextTranscript();
        transcriptReady = true;
    }
}```

### Record and transcript

```
// Space key to record and transcript
void ofApp::keyPressed(int key){
	if(key == ' '){
		if(recorder.isRecording()){
			recorder.stopRecording();
		}else{
			recorder.startRecording(ofToDataPath(ofGetTimestampString()+".wav", true));
		}
	}
}

```
