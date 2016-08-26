BasicAudioPlayers
----------------------
Chris Hummersone, 2016

These four Xcode projects are demonstrations of a simple audio file player that uses the libsndfile library to read audio files, and PortAudio to replay them.

The folders include all of the dependencies for building on OS X.

Note that even if you compile PortAudio and libsndfile on Windows, BasicAudioPlayerCallbackThreaded will not compile on Windows because the threading functions are for UNIX-like systems.

There are four projects.

1) BasicAudioPlayerBlocking

This uses PortAudio's blocking interface (via Pa_WriteStream()) to write data to the audio stream (output buffer). This technique is "blocking" in the sense that data are pushed onto the output stream, and the program blocks (waits) whilst that process is happening.

The blocking method is the simpler of the methods, but is not necessarily the most compatible.

2) BasicAudioPlayerCallback

This uses PortAudio's callback interface (notice the callback argument in the Pa_OpenStream() function call). This technique gives the responsibility of filling the output buffer to the host API - the API consumes data to fill the buffer. This is often performed in a separate thread, and that thread is often given a high priority (whereas as we have little control over priority using the blocking interface, making it less reliable).

The "callback" means that we are giving PortAudio a pointer to a function to call-back when audio data are required. In PortAudio, this function has a type: PaStreamCallback.

Importantly, the callback does not "block". (This is a good thing, because it means that our program can do other things whilst audio is consumed and processed). However, it also means that this basic program could terminate before any audio is played, because there is nothing blocking this from happening. The while()/Pa_Sleep() calls actively block the program until the file is finished being replayed.

The callback method is more complex than the blocking method, but it is also generally more compatible and more reliable.

3) BasicAudioPlayerCallbackMainBuffer

The problem with the above example is that the audio file is read inside the callback. Whilst it most likely works, this is generally considered to be bad, because such operations are blocking and take an unknown time to complete. This can cause the audio to glitch (which is surely the worst case scenario!). As stated in the PortAudio documentation, unsafe operations to put in the callback include (but are not limited to):

 * memory allocation/deallocation,
 * I/O (including file I/O as well as console I/O, such as printf()),
 * context switching (such as exec() or yield()),
 * mutex operations, or
 * anything else that might rely on the OS.

A better approach is to put such blocking operations in a separate thread. In this example, reading the audio file and writing to the ring buffer is performed by the main() thread (which is possible because main() is not blocked by the callback).

4) BasicAudioPlayerCallbackThreaded

This example is similar to 3), except that a separate POSIX thread is created to read the audio file.

In 3) there is no way to prime the ring buffer before starting the audio stream, because bufferAudioFile() is blocking (and hence must be called AFTER the stream is started - otherwise the ring buffer will never be emptied and the stream will never be started). In this example, the audio-file-reading thread can be started BEFORE starting the audio stream, because it doesn't automatically block main(), and we can block only until the ring buffer is full before starting the audio stream.

This example is based on the paex_record_file.c PortAudio example (http://www.portaudio.com/docs/v19-doxydocs/paex__record__file_8c_source.html).
