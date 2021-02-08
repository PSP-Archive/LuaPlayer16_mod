//
//  audio.h
//  Where all the real work is done
//
//  Licensed under the BSD license. See LICENSE for details.
//  Copyright (c) 2006 Daniel S. Neumeyer <dan@captaindan.org>
//

#include "audio.h"
#include "utility.h"
#include <pspaudio.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <mad.h>
#include <mp3utils.h>


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Constants ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


#define RKS_AUDIO_OUTPUT_SAMPLES (1024)
#define RKS_AUDIO_OUTPUT_BYTES   (RKS_AUDIO_OUTPUT_SAMPLES * 4)

#define RKS_AUDIO_PAUSE_SAMPLES  (11025)


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Types --------------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


typedef enum
{
	RKS_AUDIO_STOP,
	RKS_AUDIO_PAUSE,
	RKS_AUDIO_PLAY,
	RKS_AUDIO_EXIT
}
RksAudioState;


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Variables ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


volatile void *gRksAudioMP3Data     = NULL;
volatile u32 gRksAudioMP3DataLength = 0;

volatile void *gRksAudioMP3DataPtr     = NULL;
volatile u32 gRksAudioMP3DataRemaining = 0;

char *gRksAudioTitle  = NULL;
char *gRksAudioArtist = NULL;
char *gRksAudioAlbum  = NULL;
int gRksAudioTempo    = 0;

MP3SeekTable *gRksAudioSeekTable;

volatile u32 gRksAudioPosition = 0;
volatile RksAudioState gRksAudioState = RKS_AUDIO_STOP;

int gRksAudioHardwareChannel;
SceUID gRksAudioPlayThreadID;
volatile SceBool gRksAudioPlayThreadIsAsleep = 0;

volatile struct mad_stream gRksAudioMP3Stream;
volatile struct mad_frame  gRksAudioMP3Frame;
volatile struct mad_synth  gRksAudioMP3Synth;

volatile u32 gRksAudioPlaybackRate = RKS_AUDIO_RATE_UNITY;

static int myChannel;

volatile SceUID  gRksAudioSeekParamsSemaphore;
volatile SceBool gRksAudioPlayThreadShouldSeek = 0;
volatile void   *gRksAudioSeekBuffer;
volatile u32     gRksAudioSeekBufferLength;
volatile u32     gRksAudioSkipSamples;
volatile u32     gRksAudioSeekPosition;

volatile u32 gRksAudioPauseSamples = 0;


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Prototypes ---------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


s16 rksAudioScaleSample(mad_fixed_t sample);
int rksAudioPlayThread(SceSize unused1, void *unused2);


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Functions ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


void rksAudioInit(int channel)
{
	myChannel = channel;
	// Reserve audio channel.
	
	gRksAudioHardwareChannel = sceAudioChReserve(
		myChannel,
		RKS_AUDIO_OUTPUT_SAMPLES,
		PSP_AUDIO_FORMAT_STEREO);
	
	
	// Start play thread.
	
	gRksAudioSeekParamsSemaphore = sceKernelCreateSema("SeekMutex", 0, 1, 1, NULL);
	
	mad_stream_init((struct mad_stream *) &gRksAudioMP3Stream);
	mad_frame_init((struct mad_frame *) &gRksAudioMP3Frame);
	mad_synth_init((struct mad_synth *) &gRksAudioMP3Synth);
	
	gRksAudioPlayThreadID = sceKernelCreateThread(
		"Play",
		rksAudioPlayThread,
		16,
		65536,
		0,
		NULL);
	
	sceKernelStartThread(gRksAudioPlayThreadID, 0, NULL);
}


void rksAudioLoad(const char *file)
{
	// Stop playback thread.
	
	gRksAudioState = RKS_AUDIO_STOP;
	while (! gRksAudioPlayThreadIsAsleep) sceKernelDelayThread(1);
	
	mad_synth_finish((struct mad_synth *) &gRksAudioMP3Synth);
	mad_frame_finish((struct mad_frame *) &gRksAudioMP3Frame);
	mad_stream_finish((struct mad_stream *) &gRksAudioMP3Stream);
	
	mad_stream_init((struct mad_stream *) &gRksAudioMP3Stream);
	mad_frame_init((struct mad_frame *) &gRksAudioMP3Frame);
	mad_synth_init((struct mad_synth *) &gRksAudioMP3Synth);
	
	
	// Unload current audio.
	
	if (gRksAudioMP3Data)
	{
		SAFE_FREE(gRksAudioMP3Data);
		gRksAudioMP3DataLength = 0;
		
		SAFE_FREE(gRksAudioTitle);
		SAFE_FREE(gRksAudioArtist);
		SAFE_FREE(gRksAudioAlbum);
		gRksAudioTempo = 0;
		
		if (gRksAudioSeekTable)
		{
			mp3FreeSeekTable(gRksAudioSeekTable);
			gRksAudioSeekTable = NULL;
		}
		
		gRksAudioPosition = 0;
		gRksAudioPlayThreadShouldSeek = 0;
	}
	
	
	// Load new audio.
	
	int fd = open(file, O_RDONLY, 0);
	if (fd < 0) return;
	
	u32 mp3Size = lseek(fd, 0, SEEK_END);
	void *mp3Data = malloc(mp3Size + MAD_BUFFER_GUARD);
	
	if (! mp3Data)
	{
		close(fd);
		return;
	}

	lseek(fd, 0, SEEK_SET);
	read(fd, mp3Data, mp3Size);
	close(fd);
	
	void *guardPtr = mp3Data + mp3Size;
	memset(guardPtr, 0, MAD_BUFFER_GUARD);
	
	gRksAudioMP3DataLength = mp3Size;
	gRksAudioMP3Data       = mp3Data;
	
	
	// Read ID3 tag.
	
	MP3ID3Tag *tag = mp3GetID3Tag(mp3Data);
	if (tag)
	{
		gRksAudioTitle  = mp3GetTitle(tag);
		gRksAudioArtist = mp3GetArtist(tag);
		gRksAudioAlbum  = mp3GetAlbum(tag);
		gRksAudioTempo  = mp3GetTempo(tag);
	}
	
	
	// Generate seek table.
	
	gRksAudioSeekTable = mp3CreateSeekTable(mp3Data, mp3Size);
	rksAudioSeek(0);
}


u32 rksAudioDuration()
{
	return (gRksAudioSeekTable ? gRksAudioSeekTable->sampleCount : 0);
}


u32 rksAudioPosition()
{
	if (gRksAudioState == RKS_AUDIO_PAUSE)
		return gRksAudioSeekPosition;
	else
		return gRksAudioPosition - (sceAudioGetChannelRestLen(gRksAudioHardwareChannel) * gRksAudioPlaybackRate / RKS_AUDIO_RATE_UNITY);
}


void rksAudioSeek(u32 frameIndex)
{
	if (! gRksAudioSeekTable) return;
	
	if (frameIndex >= gRksAudioSeekTable->sampleCount)
		frameIndex = gRksAudioSeekTable->sampleCount - 1;
	
	
	MP3SeekTableEntry *seekEntry = mp3Seek(gRksAudioSeekTable, frameIndex);
	
	sceKernelWaitSemaCB(gRksAudioSeekParamsSemaphore, 1, 0);
	
	gRksAudioPlayThreadShouldSeek = 1;
	gRksAudioSeekBuffer           = seekEntry->bufferStart;
	gRksAudioSeekBufferLength     = seekEntry->bufferLength;
	gRksAudioSkipSamples          = frameIndex - seekEntry->sampleOffset;
	gRksAudioSeekPosition         = frameIndex;
	
	sceKernelSignalSema(gRksAudioSeekParamsSemaphore, 1);
	
	
	if (gRksAudioState == RKS_AUDIO_STOP)
		gRksAudioPosition = frameIndex;
}


void rksAudioPlay()
{
	gRksAudioState = RKS_AUDIO_PLAY;
	sceKernelWakeupThread(gRksAudioPlayThreadID);
}


void rksAudioPause()
{
	gRksAudioPauseSamples = 0;
	rksAudioSeek(rksAudioPosition());
	gRksAudioState = RKS_AUDIO_PAUSE;
}


void rksAudioStop()
{
	gRksAudioState = RKS_AUDIO_STOP;
}


SceBool rksAudioIsPlaying()
{
	return gRksAudioState != RKS_AUDIO_STOP;
}


SceBool rksAudioIsPaused()
{
	return gRksAudioState == RKS_AUDIO_PAUSE;
}


void rksAudioSetPlaybackRate(u32 fixedRate)
{
	gRksAudioPlaybackRate = fixedRate;
}


void rksAudioGetInfo(char **outTitle, char **outArtist, char **outAlbum, int *outTempo)
{
	if (outTitle)  *outTitle  = gRksAudioTitle;
	if (outArtist) *outArtist = gRksAudioArtist;
	if (outAlbum)  *outAlbum  = gRksAudioAlbum;
	if (outTempo)  *outTempo  = gRksAudioTempo;
}


s16 rksAudioScaleSample(mad_fixed_t sample)
{
/*  sample += (1L << (MAD_F_FRACBITS - 16));

  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  return sample >> (MAD_F_FRACBITS + 1 - 16);*/
  
	if (sample >=  MAD_F_ONE) return( SHRT_MAX);
	if (sample <= -MAD_F_ONE) return(-SHRT_MAX);
	
	sample = sample >> (MAD_F_FRACBITS - 15);
	return ((s16) sample);
}


// -- rksAudioPlayThread --
//
// Decodes and plays MP3 audio. Audio data begins life in the MP3 data buffer.
// It is decoded by libmad into 24-bit non-interleaved PCM. The decoded data is
// converted to 16-bit interleaved PCM and placed in the PCM buffer. Samples
// are then copied from the PCM buffer to the output triple-buffer while
// applying tempo adjustment. The output buffer is then sent to the audio
// hardware for playback.
//
// And yes, I know this is stupid.
//
int rksAudioPlayThread(SceSize unused1, void *unused2)
{
	u32 *buffer1;
	u32 *buffer2;
	u32 *buffer3;
	u32 *buffer;
	
	u32 *pcmBuffer;
	u32 *pcmBufferPtr;
	s32 pcmBufferCount;
	u32 pcmBufferCapacity;
	
//	volatile void *mp3DataPtr = NULL;
//	u32 mp3DataRemaining = 0;
	#define mp3DataPtr (gRksAudioMP3DataPtr)
	#define mp3DataRemaining (gRksAudioMP3DataRemaining)
	
	
	// Allocate rendering buffers.
	
	buffer1 = malloc(RKS_AUDIO_OUTPUT_BYTES);
	buffer2 = malloc(RKS_AUDIO_OUTPUT_BYTES);
	buffer3 = malloc(RKS_AUDIO_OUTPUT_BYTES);
	
	if (! (buffer1 && buffer2 && buffer3))
	{
		if (buffer1) free(buffer1);
		if (buffer2) free(buffer2);
		if (buffer3) free(buffer3);
		
		sceKernelExitThread(-1);
		return -1;
	}
	
	buffer1 = (u32*) (((u32) buffer1) | 0x40000000);
	buffer2 = (u32*) (((u32) buffer2) | 0x40000000);
	buffer3 = (u32*) (((u32) buffer3) | 0x40000000);
	
	buffer = buffer1;
	
	
	// Allocate PCM buffer.
	
	pcmBuffer = malloc(sizeof(u32) * 1152); // This mysterious number is the number of PCM frames hardcoded into libmad's PCM structure.

	if (! pcmBuffer)
	{
		free(buffer1);
		free(buffer2);
		free(buffer3);
		
		sceKernelExitThread(-2);
		return -2;
	}
	
	pcmBufferPtr      = pcmBuffer;
	pcmBufferCount    = 0;
	pcmBufferCapacity = 1152;
	
	
	// Go to sleep and wait for a wakeup call from the main thread.
	
	gRksAudioPlayThreadIsAsleep = 1;
	sceKernelSleepThread();
	gRksAudioPlayThreadIsAsleep = 0;
	
	
	// Rendering loop.
	
	for (;;)
	{
		// Check for state change.
		
		if (gRksAudioState == RKS_AUDIO_STOP)
		{
			gRksAudioPlayThreadIsAsleep = 1;
			sceKernelSleepThread();
			gRksAudioPlayThreadIsAsleep = 0;
			continue;
		}
		
		if (gRksAudioState == RKS_AUDIO_EXIT) break;
		
		
		// Make sure a track has been loaded.
		
		if (! gRksAudioMP3Data)
		{
			gRksAudioPlayThreadIsAsleep = 1;
			sceKernelSleepThread();
			gRksAudioPlayThreadIsAsleep = 0;
			continue;
		}
		
		if (! mp3DataPtr)
		{
			mp3DataPtr = gRksAudioMP3Data;
			mp3DataRemaining = gRksAudioMP3DataLength;
		}
		
		
		// Render audio.
		
		unsigned i = RKS_AUDIO_OUTPUT_SAMPLES;
		unsigned j = 0;
		unsigned k = 0;
		u32 *bufferPtr = buffer;
		
		while (i)
		{
			// Check for seek command.
			
			sceKernelWaitSemaCB(gRksAudioSeekParamsSemaphore, 1, 0);
			
			if (gRksAudioPlayThreadShouldSeek
				|| (gRksAudioState == RKS_AUDIO_PAUSE && gRksAudioPauseSamples >= RKS_AUDIO_PAUSE_SAMPLES))
			{
				i = RKS_AUDIO_OUTPUT_SAMPLES;
				j = 0;
				k = 0;
				bufferPtr = buffer;
				pcmBufferCount = -gRksAudioSkipSamples;
				
				mp3DataPtr = gRksAudioSeekBuffer;
				mp3DataRemaining = gRksAudioSeekBufferLength;
				
				gRksAudioPosition = gRksAudioSeekPosition;
				
				gRksAudioPlayThreadShouldSeek = 0;
				
				gRksAudioPauseSamples = 0;
			}
			
			sceKernelSignalSema(gRksAudioSeekParamsSemaphore, 1);
			
			
			// Refill PCM buffer if necessary.
			
			if (pcmBufferCount <= 0)
			{
				int pcmUnderflow = abs(pcmBufferCount);
				
				
				// Decode the next MPEG frame.
				
				mad_stream_buffer((struct mad_stream *) &gRksAudioMP3Stream, (void *) mp3DataPtr, mp3DataRemaining);
				gRksAudioMP3Stream.error = 0;
				
				if (mad_frame_decode((struct mad_frame *) &gRksAudioMP3Frame, (struct mad_stream *) &gRksAudioMP3Stream))
				{
					if (MAD_RECOVERABLE(gRksAudioMP3Stream.error))
					{
						if (gRksAudioMP3Stream.next_frame)
						{
							mp3DataRemaining -= (((u32) gRksAudioMP3Stream.next_frame) - ((u32) mp3DataPtr));
							mp3DataPtr = (void *) gRksAudioMP3Stream.next_frame;
						}
						
						continue;
					}
					else if (gRksAudioMP3Stream.error == MAD_ERROR_BUFLEN)
						gRksAudioState = RKS_AUDIO_STOP;
					
					break;
				}
				mad_synth_frame((struct mad_synth *) &gRksAudioMP3Synth, (struct mad_frame *) &gRksAudioMP3Frame);
				
				
				// Resize PCM buffer if necessary.
				
				if (pcmBufferCapacity < gRksAudioMP3Synth.pcm.length)
				{
					free(pcmBuffer);
					pcmBuffer = malloc(sizeof(u32) * gRksAudioMP3Synth.pcm.length);
					pcmBufferCapacity = gRksAudioMP3Synth.pcm.length;
				}
				
				
				// Scale samples and copy them into the PCM buffer.
				
				mad_fixed_t *left  = gRksAudioMP3Synth.pcm.samples[0];
				mad_fixed_t *right = gRksAudioMP3Synth.pcm.samples[1];
				
				pcmBufferCount = gRksAudioMP3Synth.pcm.length;
				s16 *s16PCMBufferPtr = (s16 *) pcmBuffer;
				
				while (pcmBufferCount--)
				{
					*s16PCMBufferPtr++ = rksAudioScaleSample(*left++);
					*s16PCMBufferPtr++ = rksAudioScaleSample(*right++);
				}
				
				pcmBufferCount = gRksAudioMP3Synth.pcm.length - pcmUnderflow;
				pcmBufferPtr   = pcmBuffer + pcmUnderflow;
				
				
				// Adjust MP3 buffer.
				
				if (gRksAudioMP3Stream.next_frame)
				{
					mp3DataRemaining -= (((u32) gRksAudioMP3Stream.next_frame) - ((u32) mp3DataPtr));
					mp3DataPtr = (void *) gRksAudioMP3Stream.next_frame;
				}
			}
			
			
			// Copy PCM data to playback buffer while adjusting tempo.
			
			*bufferPtr++ = *pcmBufferPtr;
			i--;
			
			j += gRksAudioPlaybackRate;
			k += j / RKS_AUDIO_RATE_UNITY;
			pcmBufferPtr   += j / RKS_AUDIO_RATE_UNITY;
			pcmBufferCount -= j / RKS_AUDIO_RATE_UNITY;
			j %= RKS_AUDIO_RATE_UNITY;
			
			gRksAudioPauseSamples++;
		}
		
		
		// Send rendered audio to hardware.
		
		sceAudioOutputBlocking(gRksAudioHardwareChannel, PSP_AUDIO_VOLUME_MAX, buffer);
		gRksAudioPosition += k;
		
		buffer = (buffer == buffer1) ? buffer2 : (buffer == buffer2) ? buffer3 : buffer1;
		
		
		// Prevent hardware from falling asleep during playback.
		
		scePowerTick(0);
	}
	
	
	// Clean up.
	
	free(buffer1);
	free(buffer2);
	free(buffer3);
	free(pcmBuffer);
	
	sceKernelExitThread(0);
	return 0;
}
