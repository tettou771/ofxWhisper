#pragma once
#include "ofMain.h"
#include "ofxSoundObjects.h"
#include "waveformDraw.h"
#include "ofxHttpUtils.h"

class ofxWhisper : public ofThread , public ofBaseSoundInput {
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
    
    // Print devices
    void printSoundDevices();
    
    // Get available sound devices
    vector<ofSoundDevice> getSoundDevices();

    // Set devide id
    void setupRecorder(int _soundDeviceID = 0);
    
    // Start recording with Device ID (default:0)
    void startRecording();
    
    // Stop recording and add recordingBuffer to audioQue
    void stopRecording();
    
    // Start realtime recording (recording continuously)
    void startRealtimeRecording();
    
    // Stop realtime recording
    void stopRealtimeRecording();
    
    // rrStartThresholdのgetterとsetter
    float getRrStartThreshold() const;
    void setRrStartThreshold(float value);

    // rrEndThresholdのgetterとsetter
    float getRrEndThreshold() const;
    void setRrEndThreshold(float value);

    // rrSilenceTimeMaxのgetterとsetter
    float getRrSilenceTimeMax() const;
    void setRrSilenceTimeMax(float value);
    
    // Add audio file to audioQue
    void transcript(string file);
    
    // Add prompt
    void setPrompt(string _prompt);
    
    string getPrompt();
    
    // set Language ISO 639-1 code
    // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
    void setLanguage(string _language);
    
    string getLanguage();
    
    // HTTP request thread
    void threadedFunction() override;
    
    // Return true if transcripts is ready
    bool hasTranscript();
    
    // Return num of transcripts
    int numTranscripts();
        
    // Get oldest transcript and remove it
    string getNextTranscript();
    
    bool isRecording();
    
    float getAudioLevel();
    
    // audio handling
    void audioIn(ofSoundBuffer &input) override;
    
    // Helper function to parse the error response and return the appropriate error code.
    ErrorCode parseErrorResponse(const ofxHttpResponse& response);
    
    // Get the error message for a given error code.
    static string getErrorMessage(ErrorCode errorCode);
    
    // Audio Level Changed Event
    struct AudioEventArgs {
        float audioLevel;
        bool isRecording;
    };
    ofEvent<AudioEventArgs> audioEvents;
    
private:
    ofSoundStream stream;
    ofxSoundRecorderObject recorder;
    
    ofEventListener recordingEndListener;
    void recordingEndCallback(string & filePath);

    // transcript history
    vector<string> transcripts;
    
    // Audio buffer que
    vector<string> audioQue;
    
    ofMutex audioQueMutex, transcriptMutex;
    
    bool recording, realtimeRecording;
    
    // OpenAI key
    string apiKey;
    
    // prompt (send to Whidper with data)
    string prompt;
    
    // language (send to Whisper with data)
    string language;
    
    ofxHttpUtils httpUtils;
    
    // Realtime recording parametors
    float rrStartThreshold, rrEndThreshold, rrSilenceTimeMax;
    uint32_t rrSilenceCoutMax, rrSilenceCount = 0;
    
    // realtime level. for audio visualizer
    float audioLevel;
    
    string getTempPath();
    
    static const int audioBufferHistoryMax = 20;
    vector<ofSoundBuffer> audioBufferHistory;
    
    // The audio file is not valid if valid buffer count less than threshold.
    int validBfferCount;
    const int validBfferCountThreshold = 20;
};
