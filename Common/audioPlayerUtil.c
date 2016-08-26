//
//  audioPlayerUtil.c
//
//  Created by Christopher Hummersone on 24/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdlib.h>
#include "audioPlayerUtil.h"

// This function opens an audio file
int openAudioFile(const char fileName[],
                    struct audioFileInfo *audioFile,
                    int maxChannels)
{
    SF_INFO sfinfo; // audio file info returned by sndfile
    
    // Open audio file
    audioFile->fileID = sf_open(fileName, SFM_READ, &sfinfo);
    
    // Pass parameters to audio file info
    audioFile->channels = sfinfo.channels;
    audioFile->sRate = sfinfo.samplerate;
    
    // Error checking
    if (audioFile->fileID == NULL)
    {
        // file not opened
        printf("An error occurred opening audio file\n");
        return ERROR_OPENING_FILE;
    }
    else if (audioFile->channels > maxChannels) {
        // number of channels exceeds channels supported by device
        printf("The specified audio file has %d audio channels. The chosen device only supports up to %d channels.\n",audioFile->channels,maxChannels);
        return INVALID_CHANNELS;
    }
    else
    {
        // everything was OK
        return NO_ERROR;
    }
}

// This function closes an audio file
void closeAudioFile(struct audioFileInfo *audioFile)
{
    // close audio file
    if (audioFile->fileID != NULL)
        sf_close(audioFile->fileID);
    
    // free malloc'd memory
    if (audioFile->buffer != NULL)
        free(audioFile->buffer);
}

// Set up output device
void getStreamParameters(PaStreamParameters *p, const char* ioName, unsigned int *maxChannels)
{
    // initial declarations
    int i,id;
    const PaDeviceInfo *info;
    const PaHostApiInfo *hostapi;
    
    // Print out a list of the devices supporting input/output
    for (i=0;i < Pa_GetDeviceCount(); i++) {
        info = Pa_GetDeviceInfo(i); // device info
        hostapi = Pa_GetHostApiInfo(info->hostApi); // API
        printf("%d: [%s] %s\n",
               i, hostapi->name, info->name);
    }
    
    // Get the user to choose an output device
    printf("Type AUDIO %s device number: \n", ioName);
    scanf("%d", &id); // this is the device id that the user typed
    info = Pa_GetDeviceInfo(id);
    hostapi = Pa_GetHostApiInfo(info->hostApi);
    printf("Opening audio %s device [%s] %s\n", ioName, hostapi->name, info->name);
    
    // Set the stream parameters
    p->device = id;
    p->suggestedLatency = Pa_GetDeviceInfo(p->device)->defaultLowOutputLatency;
    p->hostApiSpecificStreamInfo = NULL;
    
    // set maximum number of output channels
    *maxChannels = info->maxOutputChannels;
}