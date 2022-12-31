#include <windows.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

/*
	Для записи стерео сигнала пишем заголовок
	 потом пишем по 2 short данные
		Write_WAVE_Header (f,
                    44100,
                    2*8,
                    2,
                    NUM_SAMPLE);
*/


#define MAXWAVESIZE     4294967040LU


#define WRITE(f,data,len) fwrite(data,len,1,f)
/*
 *  Write 'len' Bytes to a Stream.
 *  Output Error Message if write fails
 */

static size_t
write_with_test ( FILE* outputFile, const void* data, size_t len )
{
    if ( len > 0 )
        if ( len != WRITE(outputFile, data, len) )
            printf ( "\nmppdec: write error: %s\a\n", strerror(errno) );

    return len;
}


/*
 *  Write a WAV header for a simple RIFF-WAV file with 1 PCM-Chunk. Settings are passed via function parameters.
 */

int
Write_WAVE_Header ( FILE*   outputFile,
                    DWORD  SampleFreq,
                    DWORD  BitsPerSample,
                    DWORD  Channels,
                    DWORD  SamplesPerChannel )
{
#pragma pack(1)
	struct WavHeader
	{
		DWORD riff;
		DWORD len;
		DWORD wave;
		DWORD fmt;
		DWORD pcm_length;// length of the PCM data declaration = 2+2+4+4+2+2
		WORD acm_type;  // ACM type 0x0001 = uncompressed linear PCM
		WORD cannels;// Channels
		DWORD SampleFreq;// Sample frequency
		DWORD BytesPerSecond;// Bytes per second in the data stream
		WORD BytesPerSample;// Bytes per sample time
		WORD BitsPerSample;// Bits per single sample
		DWORD data;
		DWORD pcm_data_size;// Size PCM-Daten
	} w;
#pragma pack()

    DWORD      Bytes         = (BitsPerSample + 7) / 8;
    double    PCMdataLength = (double) Channels * Bytes * SamplesPerChannel;
	size_t    ret;

	w.riff=mmioFOURCC('R','I','F','F');
	w.len=PCMdataLength + (44 - 8) < (double)MAXWAVESIZE  ?
             (DWORD)PCMdataLength + (44 - 8)  :  (DWORD)MAXWAVESIZE;
	w.wave=mmioFOURCC('W','A','V','E');
	w.fmt=mmioFOURCC('f','m','t',' ');
	w.pcm_length=0x10;
	w.acm_type=1;
	w.cannels=(WORD)Channels;
	w.SampleFreq=SampleFreq;
	w.BytesPerSecond=w.SampleFreq*Bytes*Channels;
	w.BytesPerSample=(WORD)(Bytes*Channels);
	w.BitsPerSample=(WORD)BitsPerSample;
	w.data=mmioFOURCC('d','a','t','a');
	w.pcm_data_size=PCMdataLength < MAXWAVESIZE  ?  (DWORD)PCMdataLength  :  (DWORD)MAXWAVESIZE;

/*
    BYTE   Header [44];
    BYTE*  p             = Header;
    DWORD      Bytes         = (BitsPerSample + 7) / 8;
    double    PCMdataLength = (double) Channels * Bytes * SamplesPerChannel;
    DWORD  word32;
    size_t    ret;

    *p++ = 'R';
    *p++ = 'I';
    *p++ = 'F';
    *p++ = 'F';                                   // "RIFF" label

    word32 = PCMdataLength + (44 - 8) < (double)MAXWAVESIZE  ?
             (DWORD)PCMdataLength + (44 - 8)  :  (DWORD)MAXWAVESIZE;
    *p++ = (BYTE)(word32 >>  0);
    *p++ = (BYTE)(word32 >>  8);
    *p++ = (BYTE)(word32 >> 16);
    *p++ = (BYTE)(word32 >> 24);               // Size of the next chunk

    *p++ = 'W';
    *p++ = 'A';
    *p++ = 'V';
    *p++ = 'E';                                   // "WAVE" label

    *p++ = 'f';
    *p++ = 'm';
    *p++ = 't';
    *p++ = ' ';                                   // "fmt " label

    *p++ = 0x10;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;                                  // length of the PCM data declaration = 2+2+4+4+2+2

    *p++ = 0x01;
    *p++ = 0x00;                                  // ACM type 0x0001 = uncompressed linear PCM

    *p++ = (BYTE)(Channels >> 0);
    *p++ = (BYTE)(Channels >> 8);              // Channels

    word32 = (DWORD) (SampleFreq + 0.5);
    *p++ = (BYTE)(word32 >>  0);
    *p++ = (BYTE)(word32 >>  8);
    *p++ = (BYTE)(word32 >> 16);
    *p++ = (BYTE)(word32 >> 24);               // Sample frequency

    word32 *= Bytes * Channels;
    *p++ = (BYTE)(word32 >>  0);
    *p++ = (BYTE)(word32 >>  8);
    *p++ = (BYTE)(word32 >> 16);
    *p++ = (BYTE)(word32 >> 24);               // Bytes per second in the data stream

    word32 = Bytes * Channels;
    *p++ = (BYTE)(word32 >>  0);
    *p++ = (BYTE)(word32 >>  8);               // Bytes per sample time

    *p++ = (BYTE)(BitsPerSample >> 0);
    *p++ = (BYTE)(BitsPerSample >> 8);         // Bits per single sample

    *p++ = 'd';
    *p++ = 'a';
    *p++ = 't';
    *p++ = 'a';                                   // "data" label

    word32 = PCMdataLength < MAXWAVESIZE  ?  (DWORD)PCMdataLength  :  (DWORD)MAXWAVESIZE;
    *p++ = (BYTE)(word32 >>  0);
    *p++ = (BYTE)(word32 >>  8);
    *p++ = (BYTE)(word32 >> 16);
    *p++ = (BYTE)(word32 >> 24);               // GrцЯe der rohen PCM-Daten

    assert ( p == Header + (sizeof(Header) ) );   // nix vergessen oder zuviel?
*/
	int l=sizeof(w);
    ret = write_with_test ( outputFile, &w, sizeof(w) );
    return ret;
}

/*
 *  Write PCM-Samples as 16 bit little endian Values.
 *  Problems if CHAR_BIT != 8
 */

size_t
Write_PCM_2x16bit ( FILE* fp, int* data, size_t len )
{
    size_t  ret;
    ret = write_with_test ( fp, data, 16/8 * 2 * len ) / (16/8 * 2);
    return ret;
}


/* end of wave.c */
