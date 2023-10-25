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
    whisper.getSoundDevices();
    int soundDeviceID = 0;
    whisper.setup(apiKey);
    whisper.setupRecorder(soundDeviceID);
    
    // set recording parametors
    whisper.setRrStartThreshold(0.03);
    whisper.setRrEndThreshold(0.02);
    whisper.setRrSilenceTimeMax(2.0);
    
    // set language
    //whisper.setLanguage("ja");
    
    whisper.startRealtimeRecording();
}

//--------------------------------------------------------------
void ofApp::update(){
    if (whisper.hasTranscript()) {
        string transcript = whisper.getNextTranscript();
        transcripts.push_back(transcript);
        
        // TODO
        // "transcript.txt"というファイルの末尾に、改行コードとtranscriptを追記する
        ofFile file("transcript.txt", ofFile::Append);
        if (file.is_open()) {
            file << endl << transcript; // 改行コードとtranscriptを追記
            file.close();
        } else {
            ofLogError("ofApp") << "Failed to open transcript.txt for appending";
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    int y = 60;

    if (transcripts.empty()) {
        ofSetColor(255);
        ofDrawBitmapString("Say anything.", 20, y);
    }
    else {
        for (auto text : transcripts) {
            ofSetColor(200);
            ofDrawBitmapString(text, 20, y);
            y+=16;
        }
    }
    
    // recording status
    ofRectangle bar(10, 10, 500, 20);
    ofFill();
    ofSetColor(ofColor::black);
    ofDrawRectangle(bar);
    if (whisper.isRecording()) {
        ofSetColor(ofColor::green);
    }
    else{
        ofSetColor(ofColor::red);
    }
    ofDrawRectangle(bar.x, bar.y, bar.width * whisper.getAudioLevel(), bar.height);
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
