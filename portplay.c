#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <portaudio.h>
#if _WIN32
    #pragma comment(lib, "portaudio_x86.lib")
#endif

#ifndef BOOL
    #define BOOL    int
    #define TRUE    1
    #define FALSE   0
#endif


///////////////////////////////////////////////////////////////
FILE *fp = NULL;
size_t file_size;
size_t fmt_chunk_size;
size_t data_chunk_size;
int format_id;
int channel_no;
int sampling_rate;
short  block_size;
int byte_per_sec;   
int sample_per_bit;     // 8 or 16

void wavfile_close()
{
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
}

#define TMPBUF_SIZE 256
#define CHECK_STR4(str) {           \
    size_t s;                       \
    memset(buf, 0, TMPBUF_SIZE);    \
    s = fread(buf, 1, 4, fp);       \
    if (s != 4                      \
        || buf[0] != str[0]         \
        || buf[1] != str[1]         \
        || buf[2] != str[2]         \
        || buf[3] != str[3]) {      \
            fprintf(stderr, "format error...cannot find %s\n", str);    \
            wavfile_close();        \
            return FALSE;           \
    }       \
}   

#define READ_UINT(v, str) {         \
    size_t s;                       \
    memset(buf, 0, TMPBUF_SIZE);    \
    s = fread(buf, 1, 4, fp);       \
    if (s < 4) {                    \
        fprintf(stderr, "format error...cannot read %s\n", str);    \
        wavfile_close();            \
        return FALSE;               \
    }                               \
    v = *((unsigned int *)buf);     \
    printf("%s = %d\n", str, (unsigned int)v);  \
}

#define READ_USHORT(v, str) {       \
    size_t s;                       \
    memset(buf, 0, TMPBUF_SIZE);    \
    s = fread(buf, 1, 2, fp);       \
    if (s < 2) {                    \
        fprintf(stderr, "format error...cannot read %s\n", str);    \
        wavfile_close();            \
        return FALSE;               \
    }                               \
    v = *((unsigned short *)buf);   \
    printf("%s = %d\n", str, (unsigned short)v);    \
}

BOOL wavfile_open(char *filename)
{
    size_t fmt_chunk_size;
    size_t data_chunk_size;
    char buf[TMPBUF_SIZE];

    fp = fopen(filename, "rb");
    if (fp == NULL) return FALSE;

    // header 
    CHECK_STR4("RIFF");
    READ_UINT(file_size, "file_size");
    CHECK_STR4("WAVE");

    // fmt chunk
    CHECK_STR4("fmt ");
    READ_UINT(fmt_chunk_size, "fmt_chunk_size");

    READ_USHORT(format_id, "format_id");
    READ_USHORT(channel_no,   "channel_no");
    READ_UINT(sampling_rate, "sampling_rate");
    READ_UINT(byte_per_sec, "byte_per_sec");
    READ_USHORT(block_size, "block_size");
    READ_USHORT(sample_per_bit, "sample_per_bit");
    
    // data chunk
    CHECK_STR4("data");
    READ_UINT(data_chunk_size, "data_chunk_size");
    
    return TRUE;
}

int buffer_size;
PaStream* stream;
int finish_flag = FALSE;

void print_deviceinfo()
{
    int i;
    printf("\tdevice_id list (Pa_GetDeviceInfo() result)\n");
    for (i = 0; i < Pa_GetDeviceCount(); i++) {
        const PaDeviceInfo *devinfo = Pa_GetDeviceInfo(i);
        printf("\t\t%d: %s\n", i, devinfo->name);
    }
}

int play_callback(
                const void *inputBuffer,
                void *outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData)
{
    size_t s;
    memset(outputBuffer, 0, block_size * framesPerBuffer);
    s = fread(outputBuffer, block_size, framesPerBuffer, fp);

    if (feof(fp)) {
        finish_flag = TRUE;
        return 1;
    }

    return 0;
}

int audio_start(int device_no)
{
    // initialize
    PaError             err = paNoError;
    PaStreamParameters  param_out;

    param_out.device       = device_no;
    param_out.channelCount = channel_no;
    param_out.sampleFormat = paInt16;
    param_out.suggestedLatency = Pa_GetDeviceInfo(param_out.device)->defaultLowInputLatency;
    param_out.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL,              //&inputParameters
        &param_out,        //&outputParameters
        sampling_rate,     //sample rate
        sampling_rate/10,  //frames per buffer
        paClipOff,
        play_callback,     //callback
        NULL);             //userData for callback

    if( err != paNoError ) {
        fprintf(stderr, "Pa_OpenStream() failed...\n");
        return FALSE;
    }   

    err = Pa_StartStream(stream);
    if( err != paNoError ) {
        fprintf(stderr, "Pa_StartStream() failed...\n");
        return FALSE;
    }   


    return TRUE;
}

void audio_stop()
{
    if (stream != NULL) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = NULL;
    }
}


int main(int argc, char* argv[])
{
    PaError err = paNoError;
    int device_id;
    BOOL rv;

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "Pa_Initialize() failed...\n");
        exit(1);
    }

    if (argc != 3) {
        printf("usage: %s devide_id wav_file\n\n", argv[0]);
        print_deviceinfo();
        Pa_Terminate();
        return 1;
    }

    rv = wavfile_open(argv[2]);
    if (rv == FALSE) {
        fprintf(stderr, "wav file open failed...\n");
        Pa_Terminate();
        exit(1);
    }

    device_id = atoi(argv[1]);
    rv = audio_start(device_id);
    if (rv == FALSE) {
        fprintf(stderr, "AudioDevice initialize failed...device_id=%d\n", device_id);
        Pa_Terminate();
        return 1;
    }

    while (1) {
        Pa_Sleep(100);
        if (finish_flag == TRUE) break;
    }

    audio_stop();
    Pa_Terminate();

    wavfile_close();

    return 0;
}

