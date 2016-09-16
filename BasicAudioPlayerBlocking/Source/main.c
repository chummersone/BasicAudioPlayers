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
    // Some initial declarations
    unsigned int maxChannels;               // Max channels supported by audio device
    PaStreamParameters outputParameters;    // Audio device output parameters
    PaStream *stream = NULL;                // Audio stream info
    PaError err = 0;                        // Portaudio error number
    struct audioFileInfo audioFile;         // Pass info about the audio file
    sf_count_t numberFramesRead;            // Number of frames read from audio file
    
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
    if (err != 0) goto cleanup;
    
    // Set up output device, get max output channels
    getStreamParameters(&outputParameters, OUTPUT_DEVICE, &maxChannels);
    outputParameters.sampleFormat = paFloat32; // specify output format
    
    // Open audio file
    err = openAudioFile(argv[1],&audioFile,maxChannels);
    if (err != 0) goto cleanup;
    
    // set output channels based on file
    outputParameters.channelCount = audioFile.channels;
    
    // Allocate buffer memory
    // Depends on number of channels in audio file,
    // so cannot be done until now
    audioFile.buffer = malloc(sizeof(float)*FRAMES_PER_BUFFER*(audioFile.channels));
    if (audioFile.buffer==NULL) {
        // check memory was allocated
        err = NO_MEMORY;
        printf("Insufficient memory to play audio file.\n" );
        goto cleanup;
    }
    
    // open stream for outputting audio file via callback
    err = Pa_OpenStream(&stream, NULL, &outputParameters, audioFile.sRate,
                        FRAMES_PER_BUFFER, paClipOff, NULL, &audioFile);
    if (err != 0) goto cleanup;
    
    // start playing
    err = Pa_StartStream(stream);
    if (err != 0) goto cleanup;
    
    // this is the blocking interface
    do { // read from file and write to buffer
        numberFramesRead = sf_readf_float(audioFile.fileID, audioFile.buffer, FRAMES_PER_BUFFER);
        // write buffer to stream
        err = Pa_WriteStream(stream, audioFile.buffer, numberFramesRead);
        if (err) goto cleanup;
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
    
    // all portaudio error codes are negative
    if (err<0) {
        printf("An error occured while using the portaudio stream\n" );
        printf("Error number: %d\n", err);
        printf("Error message: %s\n", Pa_GetErrorText(err));
    }
    return err;
}
