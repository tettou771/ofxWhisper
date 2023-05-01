#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetWindowTitle("example-ofxWhisper-realtimeRecord");
    
    // Load API key from config.json
    string apiKey = "";
    try {
        ofJson configJson = ofLoadJson("secret.json");
        apiKey = configJson["apiKey"];
    }
    catch (exception e) {
        ofLogError() << "No api key. Exit.";
        OF_EXIT_APP(1);
    }
    
    // Setup ofxWhisper with API key
    int soundDeviceID = 1;
    whisper.setup(apiKey);
    whisper.setupRecorder(soundDeviceID);
}

//--------------------------------------------------------------
void ofApp::update(){
    if (whisper.hasTranscript()) {
        transcripts.push_back(whisper.getNextTranscript());
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    if (whisper.isRecording()) {
        ofSetColor(255, 0, 0);
        ofDrawBitmapString("Recording. Space key to stop.", 20, 40);
    } else {
        ofSetColor(0, 220, 0);
        ofDrawBitmapString("Space key to record your voice.", 20, 40);
    }
    
    int y = 60;
    for (auto text : transcripts) {
        ofSetColor(200);
        ofDrawBitmapString(text, 20, y);
        y+=16;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' ') {
        if (whisper.isRecording()) {
            whisper.stopRecording();
        } else {
            whisper.startRecording();
        }
    }
}
