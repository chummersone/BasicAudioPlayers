//
//  main.c
//  BasicAudioPlayerBlocking
//
//  Created by Christopher Hummersone on 03/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "audioPlayerUtil.h"

// MAIN
int main(int argc, char *argv[]) {
    
    int err = 0;
    PaError err_pa = paNoError;
    
    // intial audio file info, set pointers to NULL
    struct audioFileInfo audioFile = {
        .buffer = NULL,
        .fileID = NULL
    };
    
    // program needs 1 argument: audio file name
    if (argc != 2) {
        // handle this error
        err = ERR_BAD_COMMAND_LINE;
        goto cleanup;
    }
    
    // initialise portaudio
    // go to the cleanup statement below if
    // portaudio cannot be initialized
    err_pa = Pa_Initialize();
    if (err_pa) {
        err = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // Set up output device, get max output channels
    unsigned int maxChannels; // Max channels supported by device
    PaStreamParameters outputParameters;// Audio device output parameters
    getStreamParameters(&outputParameters, OUTPUT_DEVICE, &maxChannels);
    outputParameters.sampleFormat = paFloat32; // specify output format
    
    // Open audio file
    err = openAudioFile(argv[1],&audioFile,maxChannels);
    if (err) {
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
        err = ERR_BAD_ALLOC;
        goto cleanup;
    }
    
    // open stream for outputting audio file via callback
    PaStream *stream = NULL; // Audio stream info
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        audioFile.sRate,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,
        &audioFile
    );
    if (err_pa) {
        err = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // start playing
    err_pa = Pa_StartStream(stream);
    if (err_pa) {
        err = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // this is the blocking interface
    sf_count_t numberFramesRead; // Number of frames read from audio file
    do { // read from file and write to buffer
        numberFramesRead = sf_readf_float(audioFile.fileID,
            audioFile.buffer, FRAMES_PER_BUFFER);
        // write buffer to stream
        err_pa = Pa_WriteStream(stream, audioFile.buffer, numberFramesRead);
        if (err_pa) {
            err = ERR_PORTAUDIO;
            goto cleanup;
        }
    } while (numberFramesRead > 0);
    
    // Finished playing
    printf("Finished!\n");
    
    goto cleanup;
    
cleanup:
    // make sure all the toys are put away befor exit
    
    if (stream) // close stream
        err = Pa_CloseStream(stream);
    
    // terminate portaudio
    Pa_Terminate();
    
    // close audio file
    closeAudioFile(&audioFile);
    
    // print an error msg if applicable
    printErrorMsg(err, err_pa, audioFile.fileID);
    
    return err;
}
