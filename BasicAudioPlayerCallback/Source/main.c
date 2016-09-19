//
//  main.c
//  BasicAudioPlayerCallback
//
//  Created by Christopher Hummersone on 03/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "audioPlayerUtil.h"

// Callback function passed to portaudio to play audio file
PaStreamCallback playCallback;

// MAIN
int main(int argc, char *argv[]) {
    // Some initial declarations
    unsigned int maxChannels;               // Max channels supported by device
    PaStreamParameters outputParameters;    // Audio device output parameters
    PaStream *stream = NULL;                // Audio stream info
    int err = 0;                            // Error number
    int err_cat = 0;                        // Error category
    struct audioFileInfo audioFile;         // Pass info about the audio file
    
    // intial values of audio file pointers
    audioFile.buffer = NULL;
    audioFile.fileID = NULL;
    
    // program needs 1 argument: audio file name
    if (argc != 2) {
        // handle this error
        printf("Error: Bad command line. Syntax is:\n\n");
        printf("%s filename\n",argv[0]);
        return BAD_COMMAND_LINE;
    }
    
    // initialise portaudio
    // go to the cleanup statement below if
    // portaudio cannot be initialized
    err = Pa_Initialize();
    if (err != 0) {
        err_cat = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // Set up output device, get max output channels
    getStreamParameters(&outputParameters, OUTPUT_DEVICE, &maxChannels);
    outputParameters.sampleFormat = paFloat32; // specify output format
    
    // Open audio file
    err = openAudioFile(argv[1],&audioFile,maxChannels);
    if (err != 0) {
        err_cat = ERR_SNDFILE;
        goto cleanup;
    }
    
    // set output channels based on file
    outputParameters.channelCount = audioFile.channels;
    
    // Allocate buffer memory
    // Depends on number of channels in audio file,
    // so cannot be done until now
    audioFile.buffer =
        malloc(sizeof(float)*FRAMES_PER_BUFFER*(audioFile.channels));
    if (audioFile.buffer==NULL) {
        // check memory was allocated
        err = NO_MEMORY;
        err_cat = ERR_ME;
        printf("Insufficient memory to play audio file.\n" );
        goto cleanup;
    }
    
    // open stream for outputting audio file via callback
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        audioFile.sRate,
        FRAMES_PER_BUFFER,
        paClipOff,
        playCallback,
        &audioFile
    );
    if (err != 0) {
        err_cat = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // start playing
    err = Pa_StartStream(stream);
    if (err != 0) {
        err_cat = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // wait for audio file to finish playing
    printf("Now playing...\n");
    while (Pa_IsStreamActive(stream) == 1)
        Pa_Sleep(100);
    
    // Finished playing
    printf("Finished!\n");
    
    goto cleanup;
    
cleanup:
    // make sure all the toys are put away befor exit
    
    if (stream)
        err = Pa_CloseStream(stream); // close stream
    
    // terminate portaudio
    Pa_Terminate();
    
    // close audio file
    closeAudioFile(&audioFile);
    
    // print an error msg if applicable
    printErrorMsg(err, err_cat, audioFile.fileID);
    
    return err;
}

// Callback function passed to portaudio to play audio file
int playCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    // cast inputs to correct data type
    struct audioFileInfo *data = (struct audioFileInfo *) userData;
    float *out = (float*)outputBuffer;
    
    // declare other variables
    sf_count_t numberFramesRead; // frames read from file
    
    // avoid unused variable warnings
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    
    // copy data into buffer prior to output
    numberFramesRead =
        sf_readf_float(data->fileID, data->buffer, framesPerBuffer);
    
    if (numberFramesRead>0) { // If data to read
        unsigned int i, n;
        for ( i=0; i<numberFramesRead; i++ ) { // iterature through frames
            for ( n=0; n < data->channels; n++ ) {
                // channels interleaved, so increment accesses specific channel
                *out++ = *(data->buffer)++; // write the buffer to the output
            }
        }
        return paContinue; // continue playing
    }
    else {
        return paComplete; // finished playing
    }
}
