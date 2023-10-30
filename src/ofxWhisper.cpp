#include "ofxWhisper.h"

ofxWhisper::ofxWhisper() : recording(false), realtimeRecording(false), audioLevel(0) {
    setRrStartThreshold(0.05);
    setRrEndThreshold(0.03);
    setRrSilenceTimeMax(1.0);
}

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

void ofxWhisper::printSoundDevices() {
    return ofxSoundUtils::printInputSoundDevices();
}

vector<ofSoundDevice> ofxWhisper::getSoundDevices() {
    return ofxSoundUtils::getInputSoundDevices();
}

void ofxWhisper::setupRecorder(int _soundDeviceID) {
    auto inDevices = ofxSoundUtils::getInputSoundDevices();
    
    size_t inDeviceIndex = _soundDeviceID;
    
    // Setup the sound stream.
    ofSoundStreamSettings settings;
    settings.bufferSize = 256;
    settings.numBuffers = 1;
    settings.numInputChannels = MIN(2, inDevices[inDeviceIndex].inputChannels);
    settings.numOutputChannels = MIN(2, inDevices[inDeviceIndex].outputChannels);
    settings.sampleRate = inDevices[inDeviceIndex].sampleRates[0];
    settings.setInDevice(inDevices[inDeviceIndex]);
    settings.setInListener(this);
    
    stream.setup(settings);
        
    // we register the recorder end event
    recordingEndListener = recorder.recordingEndEvent.newListener(this, &ofxWhisper::recordingEndCallback);
}

void ofxWhisper::startRecording() {
    if (!recording) {
        ofLogNotice("ofxWhisper") << "Start recording";
        recording = true;
        recorder.startRecording(getTempPath() + "recording_" + ofGetTimestampString() + ".wav", true);
        validBfferCount = 0;
    }
}

void ofxWhisper::stopRecording() {
    if (recording) {
        ofLogNotice("ofxWhisper") << "Stop recording";
        recorder.stopRecording();
    }
}

void ofxWhisper::startRealtimeRecording() {
    if (realtimeRecording) return;
    realtimeRecording = true;
    ofLogNotice("ofxWhisper") << "Start Realtime Recording";
    rrSilenceCount = 0;
}

void ofxWhisper::stopRealtimeRecording() {
    if (!realtimeRecording) return;
    if (recording) stopRecording();
    realtimeRecording = false;
    ofLogNotice("ofxWhisper") << "Stop Realtime Recording";
}

// rrStartThresholdのgetterとsetterの実装
float ofxWhisper::getRrStartThreshold() const {
    return rrStartThreshold;
}

void ofxWhisper::setRrStartThreshold(float value) {
    rrStartThreshold = MIN(MAX(value, 0), 1.);
}

// rrEndThresholdのgetterとsetterの実装
float ofxWhisper::getRrEndThreshold() const {
    return rrEndThreshold;
}

void ofxWhisper::setRrEndThreshold(float value) {
    rrEndThreshold = MIN(MAX(value, 0), 1.);
}

// rrSilenceTimeMaxのgetterとsetterの実装
float ofxWhisper::getRrSilenceTimeMax() const {
    return rrSilenceTimeMax;
}

void ofxWhisper::setRrSilenceTimeMax(float value) {
    rrSilenceTimeMax = MAX(0, value);
    rrSilenceCoutMax = rrSilenceTimeMax * stream.getSampleRate() / stream.getBufferSize();
}

void ofxWhisper::transcript(string file) {
    audioQueMutex.lock();
    audioQue.push_back(file);
    audioQueMutex.unlock();
    if (!isThreadRunning()) startThread();
}

void ofxWhisper::setPrompt(string _prompt) {
    prompt = _prompt;
}

string ofxWhisper::getPrompt() {
    return prompt;
}

void ofxWhisper::setLanguage(string _language) {
    language = _language;
}

string ofxWhisper::getLanguage() {
    return language;
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
            if (prompt != "") {
                form.addFormField("prompt", prompt);
            }
            if (language != "") {
                form.addFormField("language", language);
            }
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

float ofxWhisper::getAudioLevel() {
    return audioLevel;
}

void ofxWhisper::audioIn(ofSoundBuffer &input) {
    // calc volume
    audioLevel = 0;
    for (auto &s : input.getBuffer()) {
        audioLevel = MAX(audioLevel, abs(s));
    }
    
    // increment valid count
    if (audioLevel >= rrStartThreshold) validBfferCount++;
    
    // realtime recording control
    if (realtimeRecording) {
        // Check start
        if (!recorder.isRecording()) {
            if (audioLevel >= rrStartThreshold) {
                rrSilenceCount = 0;
                startRecording();
                
                // append past buffer history
                for (auto history : audioBufferHistory) {
                    recorder.process(history, history);
                }
            }
        }
        
        // Check end
        else {
            if (audioLevel < rrEndThreshold) {
                rrSilenceCount++;
                if (rrSilenceCount >= rrSilenceCoutMax) {
                    stopRecording();
                }
            }
            else {
                rrSilenceCount = 0;
            }
        }
    }
    
    if (recorder.isRecording()) {
        recorder.process(input, input);
    }
    
    audioBufferHistory.push_back(input);
    if (audioBufferHistory.size() >= audioBufferHistoryMax) {
        audioBufferHistory.erase(audioBufferHistory.begin());
    }
    
    // event
    AudioEventArgs args;
    args.audioLevel = audioLevel;
    args.isRecording = isRecording();
    ofNotifyEvent(audioEvents, args);
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
    ofLogNotice("ofxWhisper") << "Count: " << validBfferCount;
    if (validBfferCount >= validBfferCountThreshold) {
        transcript(filePath);
    }else{
        ofLogWarning("ofxWhisper") << "The audio is too short to transcribe.";
    }
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
