//
//  audioPlayerUtil.c
//
//  Created by Christopher Hummersone on 24/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdlib.h>
#include "audioPlayerUtil.h"

// Return a name for an input or output device
const char* getDeviceIOname(PaIOdevice ioDevice) {
    
    switch (ioDevice) {
        case INPUT_DEVICE:
            return "input";
        case OUTPUT_DEVICE:
            return "output";
        default:
            return "unkown";
    }
}

// This function opens an audio file
int openAudioFile(
    const char fileName[],
    struct audioFileInfo *audioFile,
    int maxChannels
) {
    
    SF_INFO sfinfo; // audio file info returned by sndfile
    
    // Open audio file
    audioFile->fileID = sf_open(fileName, SFM_READ, &sfinfo);
    
    // Pass parameters to audio file info
    audioFile->channels = sfinfo.channels;
    audioFile->frames = sfinfo.frames;
    audioFile->sRate = sfinfo.samplerate;
    
    // Error checking
    if (audioFile->fileID == NULL) {
        // file not opened
        printf("An error occurred opening audio file\n");
        return ERR_OPENING_FILE;
    }
    else if (audioFile->channels > maxChannels) {
        // number of channels exceeds channels supported by device
        printf("The specified audio file has %d audio channels. The chosen device only supports up to %d channels.\n",
            audioFile->channels,maxChannels);
        return ERR_INVALID_CHANNELS;
    }
    else {
        // everything was OK
        return NO_ERROR;
    }
}

// This function closes an audio file
void closeAudioFile(struct audioFileInfo *audioFile) {
    
    // close audio file
    if (audioFile->fileID != NULL)
        sf_close(audioFile->fileID);
    
    // free malloc'd memory
    if (audioFile->buffer != NULL)
        free(audioFile->buffer);
}

// Set up output device
void getStreamParameters(
    PaStreamParameters *p,
    const PaIOdevice ioDevice,
    unsigned int *maxChannels
) {
    
    // Print out a list of the devices supporting input/output
    const PaDeviceInfo *info;       // audio device info
    const PaHostApiInfo *hostapi;   // api info
    for (int i = 0;i < Pa_GetDeviceCount(); i++) {
        info = Pa_GetDeviceInfo(i); // device info
        hostapi = Pa_GetHostApiInfo(info->hostApi); // API
        if (i == Pa_GetDefaultInputDevice()) {
            printf("%d: [%s] %s (default input)\n",
                i, hostapi->name, info->name);
        }
        else if (i == Pa_GetDefaultOutputDevice()) {
            printf("%d: [%s] %s (default output)\n",
                i, hostapi->name, info->name);
        }
        else {
            printf("%d: [%s] %s\n", i, hostapi->name, info->name);
        }
    }
    
    // Get the user to choose a device
    PaDeviceIndex id; // audio device id
    while (1) {
        char* selection = NULL; // user device selection
        ssize_t read; // number of characters read from stdin
        size_t len = 0; // size of allocated memory for stdin
        printf("Type audio %s device number (press enter to use the default):\n",
            getDeviceIOname(ioDevice));
        read = getline(&selection, &len, stdin);
        if (selection[0] == '\n') { // user pressed <enter>, use default
            switch (ioDevice) {
                case INPUT_DEVICE:
                    id = Pa_GetDefaultInputDevice();
                    break;
                case OUTPUT_DEVICE:
                    id = Pa_GetDefaultOutputDevice();
                    break;
                default:
                    id = 0;
            }
        }
        else // convert str to id
            id = atoi(selection);
        
        // check id is valid
        if ((id >= 0 && id < Pa_GetDeviceCount()) && read > 0)
            break;
        else
            printf("Invalid selection!\n");
    }

    // get device info
    info = Pa_GetDeviceInfo(id);
    hostapi = Pa_GetHostApiInfo(info->hostApi);
    printf("Opening audio %s device [%s] %s\n", getDeviceIOname(ioDevice),
        hostapi->name, info->name);
    
    // Set the stream parameters
    p->device = id;
    p->suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowOutputLatency;
    p->hostApiSpecificStreamInfo = NULL;
    
    // set maximum number of output channels
    *maxChannels = info->maxOutputChannels;
}

// print an error message
void printErrorMsg(int err, int err_cat, SNDFILE *sndfile) {

    if (err) {
        switch (err_cat) {
            case ERR_ME:
                switch (err) {
                    case ERR_BAD_COMMAND_LINE:
                        puts("Bad command line syntax. The program requires one argument: the file name.\n");
                    case ERR_OPENING_FILE:
                        puts("An unknown error occurred.\n");
                    case ERR_INVALID_CHANNELS:
                        puts("The audio file contains an invalid channel count (must be moono or stereo).\n");
                    case ERR_NO_MEMORY:
                        puts("Unable to allocate memory.\n");
                    default:
                        puts("An unknown error occurred.\n");
                }
                break;
            case ERR_PORTAUDIO:
                printf("Error. %s\n", Pa_GetErrorText(err));
                break;
            case ERR_SNDFILE:
                printf("Error. %s\n", sf_strerror(sndfile));
                break;
            default:
                puts("An unknown error occurred.");
        }
    }
}

// next power of 2 (e.g. 127 -> 128)
unsigned int nextPowerOf2(unsigned int val) {
    val--;
    val = (val >> 1) | val;
    val = (val >> 2) | val;
    val = (val >> 4) | val;
    val = (val >> 8) | val;
    val = (val >> 16) | val;
    return ++val;
}
