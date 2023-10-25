#pragma once

#include "ofMain.h"
#include "ofxWhisper.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);

    ofxWhisper whisper;
    vector<string> transcripts;
};
