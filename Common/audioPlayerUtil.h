//
//  audioPlayerUtil.h
//
//  Created by Christopher Hummersone on 24/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#ifndef audioPlayerUtil_h
#define audioPlayerUtil_h

#include <portaudio.h>
#include <sndfile.h>

// How many audio frames will be pulled
// from the audio file in one go
#define FRAMES_PER_BUFFER (512)

// minimum function
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

// make enumerated constants for different errors
enum ERR_MSGS {
    NO_ERROR,
    BAD_COMMAND_LINE,
    ERROR_OPENING_FILE,
    INVALID_CHANNELS,
    NO_MEMORY
};

// struct type for storing audio file info
struct audioFileInfo {
    int         channels;   // number of audio channels
    int         sRate;      // sample rate
    SNDFILE*    fileID;     // id of audio file
    float*      buffer;     // pointer to a buffer for storing audio data
};

// This function opens an audio file
int openAudioFile(const char fileName[],
                    struct audioFileInfo *audioFile,
                    int maxChannels);

// This function closes an audio file
void closeAudioFile(struct audioFileInfo *audioFile);

// Set up audio device
void getStreamParameters(PaStreamParameters *p, const char* ioName, unsigned int *maxChannels);

#endif /* audioPlayerUtil_h */