#pragma once

#include "ofMain.h"
#include "ofxWhisper.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void dragEvent(ofDragInfo dragInfo);

    ofxWhisper whisper;
    vector<string> transcripts;
};
