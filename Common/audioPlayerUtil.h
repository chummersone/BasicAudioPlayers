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

// Type to identify input and output devices
typedef enum {
    INPUT_DEVICE,
    OUTPUT_DEVICE
} PaIOdevice;

// make enumerated constants for different errors
enum ERR_MSGS {
    NO_ERROR,
    BAD_COMMAND_LINE,
    ERROR_OPENING_FILE,
    INVALID_CHANNELS,
    NO_MEMORY
};

// make enumerated constants for different error categories
enum ERR_CAT {
    ERR_ME,
    ERR_PORTAUDIO,
    ERR_SNDFILE
};


// struct type for storing audio file info
struct audioFileInfo {
    int         channels;   // number of audio channels
    int         sRate;      // sample rate
    SNDFILE*    fileID;     // id of audio file
    float*      buffer;     // pointer to a buffer for storing audio data
};

// Return a name for an input or output device
const char* getDeviceIOname(PaIOdevice ioDevice);

// This function opens an audio file
int openAudioFile(
    const char fileName[],
    struct audioFileInfo *audioFile,
    int maxChannels
);

// This function closes an audio file
void closeAudioFile(struct audioFileInfo *audioFile);

// Set up audio device
void getStreamParameters(
    PaStreamParameters *p,
    PaIOdevice ioDevice,
    unsigned int *maxChannels
);

// print an error message
void printErrorMsg(int err, int err_cat, SNDFILE *sndfile);

#endif /* audioPlayerUtil_h */
