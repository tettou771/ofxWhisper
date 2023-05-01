#pragma once

#include "ofMain.h"
#include "ofxWhisper.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();

    ofxWhisper whisper;
    string transcript;
    bool transcriptReady;
};
