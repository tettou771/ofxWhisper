#include "ofxWhisper.h"

ofxWhisper::ofxWhisper() : recording(false) {}

ofxWhisper::~ofxWhisper() {
    stopThread();
    waitForThread();
    
    // delete wav files
    string path = getTempPath();
    if (ofFile(path).isDirectory()) {
        auto d = ofDirectory(path);
        d.remove(true);
    }
}

void ofxWhisper::setup(string api_key) {
    apiKey = api_key;
    
    if (!ofFile(getTempPath()).exists()){
        ofDirectory(getTempPath()).create();
    }
}

void ofxWhisper::setupRecorder(int _soundDeviceID) {
    ofxSoundUtils::printInputSoundDevices();
    
    auto inDevices = ofxSoundUtils::getInputSoundDevices();
    auto outDevices = ofxSoundUtils::getOutputSoundDevices();
    // IMPORTANT!!!
    // The following line of code is where you set which audio interface to use.
    // the index is the number printed in the console inside [ ] before the interface name
    
    size_t inDeviceIndex = _soundDeviceID;
    size_t outDeviceIndex = 0;
    
    // Setup the sound stream.
    ofSoundStreamSettings settings;
    settings.bufferSize = 256;
    settings.numBuffers = 1;
    settings.numInputChannels =  inDevices[inDeviceIndex].inputChannels;
    settings.numOutputChannels = outDevices[outDeviceIndex].outputChannels;
    settings.sampleRate = inDevices[inDeviceIndex].sampleRates[0];
    settings.setInDevice(inDevices[inDeviceIndex]);
    settings.setOutDevice(outDevices[outDeviceIndex]);
    
    stream.setup(settings);
    
    // link the sound stream with the ofxSoundObjects
    input.setInputStream(stream);
    output.setOutputStream(stream);
    
    //we will use a mixer right before the output, just to mute out the output and avoid the nasty feedback you get otherwise. This is kind of a hack and eventually it would be unnecesary. You can add add a GUI to the mixer if you want to. Look at the mixer example.
    mixer.setMasterVolume(0);
    
    //Create objects signal chain.
    //Currently you need to connect the recorder to the output, because of the pull-through audio architecture being used. Eventually this need would become unnecesary.
    input.connectTo(wave).connectTo(recorder).connectTo(mixer).connectTo(output);

    
    // we register the recorder end event
    recordingEndListener = recorder.recordingEndEvent.newListener(this, &ofxWhisper::recordingEndCallback);
}

void ofxWhisper::startRecording() {
    if (!recording) {
        ofLogNotice("ofxWhisper") << "Start recording";
        recording = true;
        recorder.startRecording(getTempPath() + "recording_" + ofGetTimestampString() + ".wav", true);
    }
}

void ofxWhisper::stopRecording() {
    if (recording) {
        ofLogNotice("ofxWhisper") << "Stop recording";
        recorder.stopRecording();
    }
}

void ofxWhisper::transcript(string file) {
    audioQueMutex.lock();
    audioQue.push_back(file);
    audioQueMutex.unlock();
    if (!isThreadRunning()) startThread();
}

void ofxWhisper::threadedFunction() {
    while (isThreadRunning()) {
        bool hasData = false;
        string soundFilePath = "";
        ofxSoundFile soundFile;
        ofSoundBuffer buffer;
        audioQueMutex.lock();
        if (!audioQue.empty()) {
            soundFilePath = audioQue.front();
            audioQue.erase(audioQue.begin());
        }
        audioQueMutex.unlock();

        if (soundFilePath != "") {
            int tryCount = 0;
            while (!ofFile(soundFilePath).exists()) {
                if (tryCount++ == 10) break;
                ofSleepMillis(100);
            }
            
            if (ofFile(soundFilePath).exists()) {
                hasData = true;
            } else {
                ofLogError("ofxWhisper") << "Data " << soundFilePath << " is not exists.";
            }
        }

        if (hasData) {
            ofxHttpForm form;
            form.action = "https://api.openai.com/v1/audio/transcriptions";
            form.method = OFX_HTTP_POST;
            form.addHeaderField("Authorization", "Bearer " + apiKey);
            form.addHeaderField("Content-Type", "multipart/form-data");
            form.addFile("file", soundFilePath);
            form.addFormField("model", "whisper-1");

            ofxHttpResponse response = httpUtils.submitForm(form);
            
            auto errorCode = parseErrorResponse(response);
                                    
            if (errorCode == Success) {
                string rawText = response.responseBody.getText();
                ofJson res = ofJson::parse(rawText);
                try {
                    string transcript = res["text"];
                    ofLogVerbose("ofxWhisper") << "Got transcript: " << transcript;
                    transcriptMutex.lock();
                    transcripts.push_back(transcript);
                    transcriptMutex.unlock();
                }
                catch (exception e) {
                    ofLogError("ofxWhisper") << "JOSN parse error";
                }
            } else {
                ofLogError("ofxWhisper") << getErrorMessage(errorCode);
                ofLogVerbose("ofxWhisper") << "Data: " << response.responseBody.getText();
                transcriptMutex.lock();
                transcripts.push_back("Error");
                transcriptMutex.unlock();
            }
        } else {
            // thread end
            break;
        }
    }
}

bool ofxWhisper::hasTranscript() {
    transcriptMutex.lock();
    bool has_transcript = !transcripts.empty();
    transcriptMutex.unlock();
    return has_transcript;
}

int ofxWhisper::numTranscripts() {
    audioQueMutex.lock();
    int num_transcripts = (int)transcripts.size();
    audioQueMutex.unlock();
    return num_transcripts;
}

string ofxWhisper::getNextTranscript() {
    string transcript;
    transcriptMutex.lock();
    if (!transcripts.empty()) {
        transcript = transcripts.front();
        transcripts.erase(transcripts.begin());
    }
    transcriptMutex.unlock();
    return transcript;
}

bool ofxWhisper::isRecording() {
    return recording;
}

// Helper function to parse the error response and return the appropriate error code.
ofxWhisper::ErrorCode ofxWhisper::parseErrorResponse(const ofxHttpResponse& response) {
    int status = response.status;
    if (status == 200) {
        return Success;
    }
    else if (status == 401) {
        return InvalidAPIKey;
    } else if (status >= 500 && status < 600) {
        return ServerError;
    } else if (status == 429) {
        return RateLimitExceeded;
    } else if (status == 400) {
        ofJson errorJson = ofJson::parse(response.responseBody);
        string errorType = errorJson["error"]["type"].get<std::string>();
        if (errorType == "model_not_found") {
            return InvalidModel;
        } else if (errorType == "too_many_tokens") {
            return TokenLimitExceeded;
        } else {
            return BadRequest;
        }
    } else if (status == 408) {
        return Timeout;
    } else {
        return UnknownError;
    }
}

string ofxWhisper::getErrorMessage(ofxWhisper::ErrorCode errorCode) {
    switch (errorCode) {
        case Success:
            return "Success";
        case InvalidAPIKey:
            return "Invalid API key";
        case NetworkError:
            return "Network error";
        case ServerError:
            return "Server error";
        case RateLimitExceeded:
            return "Rate limit exceeded";
        case TokenLimitExceeded:
            return "Token limit exceeded";
        case InvalidModel:
            return "Invalid model";
        case BadRequest:
            return "Bad request";
        case Timeout:
            return "Timeout";
        default:
            return "Unknown error";
    }
}

void ofxWhisper::recordingEndCallback(string &filePath) {
    ofLogNotice("ofxWhisper") << "Recording end. " << filePath;
    transcript(filePath);
    recording = false;
}

string ofxWhisper::getTempPath() {
   char tempPath[PATH_MAX];
   size_t tempPathSize = confstr(_CS_DARWIN_USER_TEMP_DIR, tempPath, sizeof(tempPath));
   
   if (tempPathSize == 0 || tempPathSize > sizeof(tempPath)) {
       return "";
   }
   
   return ofToDataPath(string(tempPath) + "ofxWhisper/");
}
