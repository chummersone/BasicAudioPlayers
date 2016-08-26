//
//  audioPlayerUtil.c
//
//  Created by Christopher Hummersone on 24/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdlib.h>
#include "audioPlayerUtil.h"

// Return a name for an input or output device
const char* getDeviceIOname(PaIOdevice ioDevice)
{
    switch (ioDevice)
    {
        case INPUT_DEVICE:
            return "input";
        case OUTPUT_DEVICE:
            return "output";
        default:
            return "unkown";
    }
}

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
void getStreamParameters(PaStreamParameters *p, PaIOdevice ioDevice, unsigned int *maxChannels)
{
    // initial declarations
    PaDeviceIndex id;
    const PaDeviceInfo *info;
    const PaHostApiInfo *hostapi;
    char* selection = NULL;
    ssize_t read;
    size_t len = 0;
    
    // Print out a list of the devices supporting input/output
    for (int i = 0;i < Pa_GetDeviceCount(); i++) {
        info = Pa_GetDeviceInfo(i); // device info
        hostapi = Pa_GetHostApiInfo(info->hostApi); // API
        if (i == Pa_GetDefaultInputDevice())
            printf("%d: [%s] %s (default input)\n", i, hostapi->name, info->name);
        else if (i == Pa_GetDefaultOutputDevice())
            printf("%d: [%s] %s (default output)\n", i, hostapi->name, info->name);
        else
            printf("%d: [%s] %s\n", i, hostapi->name, info->name);
    }
    
    // Get the user to choose a device
    while (1)
    {
        printf("Type audio %s device number (press enter to use the default):\n",
               getDeviceIOname(ioDevice));
        read = getline(&selection, &len, stdin);
        if (selection[0] == '\n')
        { // user pressed <enter>, use default
            switch (ioDevice)
            {
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
    printf("Opening audio %s device [%s] %s\n", getDeviceIOname(ioDevice), hostapi->name, info->name);
    
    // Set the stream parameters
    p->device = id;
    p->suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowOutputLatency;
    p->hostApiSpecificStreamInfo = NULL;
    
    // set maximum number of output channels
    *maxChannels = info->maxOutputChannels;
}