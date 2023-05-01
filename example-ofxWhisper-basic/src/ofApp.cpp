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
}

void ofApp::update() {
    // Check if the transcript is ready
    if (whisper.hasTranscript()) {
        transcripts.push_back(whisper.getNextTranscript());
    }
}

void ofApp::draw() {
    ofSetColor(0, 220, 0);
    ofDrawBitmapString("Drag and drop audio file here.\nCompatible files: m4a, mp3, mp4, mpeg, mpga, wav, webm. size < 25MB", 20, 40);
    
    int y = 80;
    for (auto text : transcripts) {
        ofSetColor(200);
        ofDrawBitmapString(text, 20, y);
        y+=16;
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    for (auto file : dragInfo.files) {
        // Load and transcribe the audio file
        whisper.transcript(file);
    }
}
