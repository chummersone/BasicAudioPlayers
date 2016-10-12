//
//  main.c
//  BasicAudioPlayerCallbackMainBuffer
//
//  Created by Christopher Hummersone on 03/08/2016.
//  Copyright © 2016 Christopher Hummersone. All rights reserved.
//

#include <stdio.h>
#include <pa_ringbuffer.h>
#include <pa_util.h>
#include "audioPlayerUtil.h"

// Constants
#define NUM_WRITES_PER_BUFFER (4)

// struct type for storing audio file and other thread info
struct threadData {
    struct audioFileInfo    audioFile;
    unsigned short          readComplete;
    int                     threadSyncFlag;
    sf_count_t              frameCount;
    float                   *ringBufferData;
    PaUtilRingBuffer        ringBuffer;
    pthread_t               threadHandle;
};

// Callback function passed to portaudio to play audio file
PaStreamCallback playCallback;

// Thread functions
void bufferAudioFile(struct threadData* pData);

// function for calculating the next power of 2
unsigned int nextPowerOf2(unsigned int val);

// MAIN
int main(int argc, char *argv[]) {
    
    int err = 0;        // Error number
    int err_cat = 0;    // Error category
    
    // info about the audio file and thread
    struct threadData pData = {
        .audioFile.buffer = NULL,
        .audioFile.fileID = NULL,
        .threadSyncFlag = 0,
        .readComplete = 0,
        .frameCount = 0,
        .ringBufferData = NULL
    };
    
    // program needs 1 argument: audio file name
    if (argc != 2) {
        // handle this error
        err_cat = ERR_ME;
        err = ERR_BAD_COMMAND_LINE;
        goto cleanup;
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
    unsigned int maxChannels; // Max channels supported by device
    PaStreamParameters outputParameters; // Audio device output parameters
    getStreamParameters(&outputParameters, OUTPUT_DEVICE, &maxChannels);
    outputParameters.sampleFormat = paFloat32; // specify output format
    
    // Open audio file
    err = openAudioFile(argv[1],&pData.audioFile,maxChannels);
    if (err != 0) {
        err_cat = ERR_SNDFILE;
        goto cleanup;
    }
    
    // set output channels based on file
    outputParameters.channelCount = pData.audioFile.channels;
    
    // allocate ring buffer memory
    unsigned int numSamples = nextPowerOf2((unsigned)
        (pData.audioFile.sRate * 0.5 * pData.audioFile.channels)
    ); // Number of samples in ring buffer
    pData.ringBufferData =
        (float *) PaUtil_AllocateMemory(sizeof(float)*numSamples);
    
    if(pData.ringBufferData == NULL) {
        // check memory was allocated
        err = ERR_NO_MEMORY;
        err_cat = ERR_ME;
        goto cleanup;
    }
    
    // initialise ring buffer
    err = PaUtil_InitializeRingBuffer(
        &pData.ringBuffer,
        sizeof(float),
        numSamples,
        pData.ringBufferData
    );
    if (err != 0) {
        err_cat = ERR_PORTAUDIO;
        goto cleanup;
    }
    
    // open stream for outputting audio file via callback
    PaStream *stream = NULL; // Audio stream info
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        pData.audioFile.sRate,
        FRAMES_PER_BUFFER,
        paClipOff,
        playCallback,
        &pData
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
    
    // start putting audio data on to the ring buffer
    printf("Now playing...\n");
    bufferAudioFile(&pData);
    
    // wait for audio file to finish playing
    while (Pa_IsStreamActive(stream) == 1)
        Pa_Sleep(100);
    
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
    closeAudioFile(&pData.audioFile);
    
    // free allocated memory
    if (pData.ringBufferData != NULL)
        PaUtil_FreeMemory(pData.ringBufferData);
    
    // print an error msg if applicable
    printErrorMsg(err, err_cat, pData.audioFile.fileID);
    
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
    
    // cast inputs to appropriate types
    struct threadData *data = (struct threadData *) userData;
    float *out = (float*) outputBuffer;

    // determine how many elements to pass to output buffer
    ring_buffer_size_t elementsToPlay =
        PaUtil_GetRingBufferReadAvailable(&data->ringBuffer);
    ring_buffer_size_t elementsToRead = min(
        elementsToPlay,
        (ring_buffer_size_t)(framesPerBuffer * data->audioFile.channels)
    );
    
    // prevent unused variable warnings
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;
    
    // read data from buffer and put in output buffer
    PaUtil_ReadRingBuffer(&data->ringBuffer, out, elementsToRead);
    
    if (data->readComplete == 1 && elementsToPlay == 0)
        return paComplete; // finished reading file
    else
        return paContinue; // still reading file
        
}

// This routine is run in a separate thread to read data from file into the ring
// buffer. When the file has reached the end, a flag is set so that the PA
// callback can return paComplete.
void bufferAudioFile(struct threadData* pData) {
    while (1) {
        // how many elements can be written
        ring_buffer_size_t numAvailableElements =
            PaUtil_GetRingBufferWriteAvailable(&pData->ringBuffer);
        
        if (numAvailableElements >=
            pData->ringBuffer.bufferSize / NUM_WRITES_PER_BUFFER) {
            // there is space for writing
            
            void* ptr[2] = {0};
            ring_buffer_size_t sizes[2] = {0};
            
            // Get region of ring buffer for writing
            PaUtil_GetRingBufferWriteRegions(
                &pData->ringBuffer,
                numAvailableElements,
                ptr + 0,
                sizes + 0,
                ptr + 1,
                sizes + 1
            );
            
            // now get data from file and write to buffer
            ring_buffer_size_t itemsReadFromFile = 0;
            for (int i = 0; i < 2 && ptr[i] != NULL; ++i) {
                if (sizes[i] % pData->audioFile.channels) {
                    // ensure items is integer number of channels
                    sizes[i] -= sizes[i] % pData->audioFile.channels;
                }
                itemsReadFromFile += (ring_buffer_size_t)
                    sf_read_float(pData->audioFile.fileID, ptr[i], sizes[i]);
            }
            
            // advance write index
            PaUtil_AdvanceRingBufferWriteIndex(
                &pData->ringBuffer,
                itemsReadFromFile
            );
            
            if (itemsReadFromFile > 0) {
                // Check current position against file length; use that to
                // determine whether the read is complete
                pData->frameCount +=
                    itemsReadFromFile/(pData->audioFile.channels);
                if (pData->frameCount == pData->audioFile.frames) {
                    pData->readComplete = 1;
                }
            }
            else {
                // No data to read
                pData->threadSyncFlag = 1;
                break;
            }
        }
        
        // Sleep a little while...
        Pa_Sleep(20);
        // Then check if we need to fill the buffer
    }
}
