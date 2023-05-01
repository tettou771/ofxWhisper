#include "ofApp.h"

void ofApp::setup() {
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetWindowTitle("example-ofxWhisper-basic");

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
    whisper.setup(apiKey);

    // Load and transcribe the audio file
    whisper.transcript("sample_speech.mp3");

    transcriptReady = false;
}

void ofApp::update() {
    // Check if the transcript is ready
    if (whisper.hasTranscript()) {
        transcript = whisper.getNextTranscript();
        transcriptReady = true;
    }
}

void ofApp::draw() {
    ofBackground(0);
    ofSetColor(255);

    if (transcriptReady) {
        // Draw the transcript text on the screen
        ofDrawBitmapString("Transcript:\n" + transcript, 20, 40);
    } else {
        // Draw "Waiting for transcript..." on the screen
        ofDrawBitmapString("Waiting for transcript...", 20, 40);
    }
}
