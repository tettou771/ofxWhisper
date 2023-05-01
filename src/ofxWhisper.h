#pragma once
#include "ofMain.h"
#include "ofxSoundObjects.h"
#include "waveformDraw.h"
#include "ofxHttpUtils.h"

class ofxWhisper : public ofThread {
public:
    ofxWhisper();
    ~ofxWhisper();
    
    // Error codes for various error conditions.
    enum ErrorCode {
        Success = 0,
        InvalidAPIKey,
        NetworkError,
        ServerError,
        RateLimitExceeded,
        TokenLimitExceeded,
        InvalidModel,
        BadRequest,
        Timeout,
        UnknownError
    };

    void setup(string api_key);
    
    void setupRecorder(int _soundDeviceID = 0);
    
    // Start recording with Device ID (default:0)
    void startRecording();
    
    // Stop recording and add recordingBuffer to audioQue
    void stopRecording();
    
    // Add audio file to audioQue
    void transcript(string file);
    
    // HTTP request thread
    void threadedFunction() override;
    
    // Return true if transcripts is ready
    bool hasTranscript();
    
    // Return num of transcripts
    int numTranscripts();

    // Get oldest transcript and remove it
    string getNextTranscript();
    
    bool isRecording();
    
    // Helper function to parse the error response and return the appropriate error code.
    ErrorCode parseErrorResponse(const ofxHttpResponse& response);
    
    // Get the error message for a given error code.
    static string getErrorMessage(ErrorCode errorCode);
    
private:
    waveformDraw wave;
    ofxSoundInput input;
    ofxSoundOutput output;
    ofSoundStream stream;
    ofxSoundMixer mixer;
    ofxSoundRecorderObject recorder;
    
    ofEventListener recordingEndListener;
    void recordingEndCallback(string & filePath);

    // transcript history
    vector<string> transcripts;
    
    // Audio buffer que
    vector<string> audioQue;
    
    ofMutex audioQueMutex, transcriptMutex;
    
    bool recording;
    
    // OpenAI key
    string apiKey;
    
    ofxHttpUtils httpUtils;
    
    string getTempPath();
};
