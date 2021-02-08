#include <pspkernel.h>
#include <pspaudio.h>
#include <psppower.h>
#include <string.h>

#include "mp3.h"
#include <mad.h>

#define printf(fmt, ...)
#define debug_log(fmt, ...)

//#define NOEXIT 1


volatile int MP3Thread_Active = 1;

char MP3_file[256];
int MP3_newfile,MP3_playing;
int MP3_pause;
int MP3_file_offset;
int MP3_resume;

char MP3_dir[MAXPATH];
char MP3_playlist[MAX_PLAYLIST][DIR_PATHLEN+1];
int MP3_playlist_cnt, MP3_playlist_cur;
int MP3_track=0;

int mp3_thread=-1;
int loop_track=0;
SceUID f;
static int eos;

#ifdef NOEXIT
int MP3_resident=1;
#endif
#define OUTPUT_BUFFER_SIZE	1024*4 /* Must be an integer multiple of 4. */
#define INPUT_BUFFER_SIZE	(2*OUTPUT_BUFFER_SIZE)

#define MAXVOLUME	0x8000
int mp3_volume = 100;
int mp3_buffer_size=OUTPUT_BUFFER_SIZE/4;
static int mp3_handle;

static unsigned char		InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD],
						OutputBuffer[2][OUTPUT_BUFFER_SIZE],
						*OutputPtr=OutputBuffer[0],
						*GuardPtr=NULL;
volatile int OutputBuffer_flip=0;
volatile static const unsigned char	*OutputBufferEnd=OutputBuffer[0]+OUTPUT_BUFFER_SIZE;
static struct mad_stream	Stream;
static struct mad_frame	Frame;
static struct mad_synth	Synth;
static mad_timer_t	Timer;


void psp_OpenAudio() {
	mp3_handle = sceAudioChReserve( 7, mp3_buffer_size, 0 );
	if (mp3_handle < 0)
		mp3_handle = sceAudioChReserve( PSP_AUDIO_NEXT_CHANNEL, mp3_buffer_size, 0 );
}

void psp_CloseAudio() {
	sceAudioChRelease( mp3_handle );

	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

}

int irsmp3Status(void)
{
        if (MP3Thread_Active && MP3_playing) {
                if (MP3_pause)
                        return(MP3_STATUS_PAUSE);
                else
                        return(MP3_STATUS_PLAY);
	}
        return(0);
}


void irsmp3Pause(int pause){
	if (mp3_thread==-1) return;
	MP3_pause=pause;
	if (pause) {debug_log("pause");sceKernelSuspendThread(mp3_thread);}
	else {debug_log("resume");sceKernelResumeThread(mp3_thread);}
}

void irsmp3Resume()
{
	if (MP3Thread_Active && MP3_playing) {
		f = sceIoOpen(MP3_file, PSP_O_RDONLY, 0777);
		sceIoLseek(f, MP3_file_offset, PSP_SEEK_SET);
		irsmp3Pause(MP3_RESUME);
	}
}

void irsmp3Suspend()
{
	if (MP3Thread_Active && MP3_playing) {
		irsmp3Pause(MP3_PAUSE);
		MP3_file_offset = sceIoLseek(f, 0, PSP_SEEK_CUR);
		sceIoClose(f);
	}
}

void irsmp3ASuspend()
{
	if (MP3Thread_Active && MP3_playing) {
		//irsmp3Pause(MP3_PAUSE);
		MP3_file_offset = 0;
		sceIoClose(f); f=-1;
	}
}

void irsmp3Play(char *name, int playmode, int offset)
{
	eos = 0;
	if (playmode == MP3_PLAY)
		loop_track = 0;
	else if (playmode == MP3_REPEAT)
		loop_track = 1;
	strcpy(MP3_file, name);
	MP3_playlist_cnt = 0;
	MP3_playlist_cur = 0;
	MP3_track = 0;
	MP3_newfile=1;
	MP3_playing=0;
	MP3_file_offset = offset;
	if (MP3_file_offset)
		MP3_resume = 1;
	else
		MP3_resume = 0;
	sceKernelWakeupThread(mp3_thread);
	irsmp3Pause(0);
}

void irsmp3Volume(int vol)
{
	mp3_volume = vol;
}

void irsmp3Stop()
{
	irsmp3ASuspend();
	MP3_playing = 0;
	psp_CloseAudio();
	eos = 0;
}

void irsmp3Time(char *dest)
{
    mad_timer_string(Timer, dest, "%02lu:%02u:%02u", MAD_UNITS_HOURS, MAD_UNITS_MILLISECONDS, 0);
}

int irsmp3EndOfStream()
{
    if (eos == 1)
	return 1;
    return 0;
}

void irsmp3Exit()
{
#ifdef NOEXIT
	MP3_resident = 0;
#endif
}

static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The signed short value is formed, after clipping, by the least
	 * significant whole part bit, followed by the 15 most significant
	 * fractional part bits. Warning: this is a quick and dirty way to
	 * compute the 16-bit number, madplay includes much better
	 * algorithms.
	 */

	/* Clipping */
	if(Fixed>=MAD_F_ONE)
		return(32767);
	if(Fixed<=-MAD_F_ONE)
		return(-32767);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}


int irsmp3_thread(SceSize args, void *argp){
	int filesize=0;
	int Status, i;
	int offset;
	unsigned long FrameCount=0;

	/* First the structures used by libmad must be initialized. */


	/* Decoding options can here be set in the options field of the
	 * Stream structure.
	 */

	/* {1} When decoding from a file we need to know when the end of
	 * the file is reached at the same time as the last bytes are read
	 * (see also the comment marked {3} bellow). Neither the standard
	 * C fread() function nor the POSIX read() system call provides
	 * this feature. We thus need to perform our reads through an
	 * interface having this feature, this is implemented here by the
	 * bstdfile.c module.
	 */
	//SetupCallbacks();
	sceKernelSleepThread();
	psp_OpenAudio();
	f=-1;
#ifdef NOEXIT
while (MP3_resident) {
	MP3Thread_Active = 1;
#endif
	while (MP3Thread_Active) {
		if (MP3_playlist_cnt) {
			if (MP3_playlist_cur >= MP3_playlist_cnt) {
				if (loop_track)
					MP3_playlist_cur = 0;
				else
					break;
			}
			strcpy(MP3_file, MP3_dir);
			strcat(MP3_file, MP3_playlist[MP3_playlist_cur]);
			MP3_playlist_cur++;
			MP3_newfile = 1;
		}
		if (MP3_newfile) {
			MP3_newfile = 0;
			f = sceIoOpen(MP3_file, PSP_O_RDONLY, 0777);
			if (f < 0) {
				MP3_playing=0;
				debug_log(MP3_file);
				MP3Thread_Active = 0;
				break;
			}
			debug_log("mp3 file opened");

			filesize = sceIoLseek(f, 0, PSP_SEEK_END);
			if (MP3_resume) {
				MP3_resume = 0;
				offset = MP3_file_offset;
				filesize -= MP3_file_offset;
			} else
				offset = 0;
			sceIoLseek(f, offset, PSP_SEEK_SET);
			/*init mad*/
			mad_stream_init(&Stream);
			mad_frame_init(&Frame);
			mad_synth_init(&Synth);
			mad_timer_reset(&Timer);
			/*var*/
			FrameCount = 0;
			Status = 0;
			OutputBuffer_flip = 0;
			OutputPtr = OutputBuffer[0];
			OutputBufferEnd = OutputBuffer[0] + OUTPUT_BUFFER_SIZE;
			MP3_playing = 1;

		} else
			break;
		/* This is the decoding loop. */
		while (MP3_playing && MP3Thread_Active)	{
			// input
			if (Stream.buffer == NULL || Stream.error == MAD_ERROR_BUFLEN)	{
				size_t ReadSize, Remaining;
				unsigned char *ReadStart;
				if (Stream.next_frame != NULL) {
					Remaining=Stream.bufend-Stream.next_frame;
					memmove(InputBuffer,Stream.next_frame,Remaining);
					ReadStart=InputBuffer+Remaining;
					ReadSize=INPUT_BUFFER_SIZE-Remaining;
				} else {
					ReadSize=INPUT_BUFFER_SIZE;
					ReadStart=InputBuffer;
					Remaining=0;
				}
				// Fill-in the buffer.
				ReadSize=sceIoRead(f, ReadStart, ReadSize);
				if (ReadSize >= 0) filesize-=ReadSize;
				if ((MP3_track == -1) || (MP3_track == 1)) {
					filesize = 0;
					if (MP3_track == -1) {
						MP3_playlist_cur -= 2;
						if (MP3_playlist_cur < 0)
							MP3_playlist_cur = 0;
					}
					MP3_track = 0;
				}
				if(filesize==0)	{
					if (loop_track && !MP3_playlist_cnt){
						debug_log("loop");
						filesize = sceIoLseek(f, 0, PSP_SEEK_END);
						sceIoLseek(f, 0, PSP_SEEK_SET);
						eos = 1;
					} else {
						MP3_playing=0;  //no break to decode last frame
						//break;
					}
				}
			/* {3} When decoding the last frame of a file, it must be
			 * followed by MAD_BUFFER_GUARD zero bytes if one wants to
			 * decode that last frame. When the end of file is
			 * detected we append that quantity of bytes at the end of
			 * the available data. Note that the buffer can't overflow
			 * as the guard size was allocated but not used the the
			 * buffer management code. (See also the comment marked
			 * {1}.)
			 *
			 * In a message to the mad-dev mailing list on May 29th,
			 * 2001, Rob Leslie explains the guard zone as follows:
			 *
			 *    "The reason for MAD_BUFFER_GUARD has to do with the
			 *    way decoding is performed. In Layer III, Huffman
			 *    decoding may inadvertently read a few bytes beyond
			 *    the end of the buffer in the case of certain invalid
			 *    input. This is not detected until after the fact. To
			 *    prevent this from causing problems, and also to
			 *    ensure the next frame's main_data_begin pointer is
			 *    always accessible, MAD requires MAD_BUFFER_GUARD
			 *    (currently 8) bytes to be present in the buffer past
			 *    the end of the current frame in order to decode the
			 *    frame."
			 */

				if (!filesize && ReadSize > 0) {
					GuardPtr=ReadStart+ReadSize;
					memset(GuardPtr,0,MAD_BUFFER_GUARD);
					ReadSize+=MAD_BUFFER_GUARD;
				}
				// decode
				if (ReadSize + Remaining > 0)
					mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
				Stream.error=0;
			}
			if (mad_frame_decode(&Frame,&Stream)) {
				if (MAD_RECOVERABLE(Stream.error)) {
					//if (Stream.error!=MAD_ERROR_LOSTSYNC || Stream.this_frame != GuardPtr) {
						//debug_log("recov. error");
				    		//MP3_playing = 0;
				    		// alert("error",mad_stream_errorstr(&Stream));
						//sceDisplayWaitVblankStart();
					//}
					continue;
				} else if (Stream.error == MAD_ERROR_BUFLEN) {
					//if (FrameCount)
						continue;
					//else
						//break;
				} else {
					// unrecov. error
					Status=1;
					break;
			 	}
	  		}

		/* The characteristics of the stream's first frame is printed
		 * on stderr. The first frame is representative of the entire
		 * stream.
		 */
		/*if(FrameCount==0)
			if(PrintFrameInfo(stderr,&Frame.header))
			{
				Status=1;
				break;
			}*/

		/* Accounting. The computed frame duration is in the frame
		 * header structure. It is expressed as a fixed point number
		 * whole data type is mad_timer_t. It is different from the
		 * samples fixed point format and unlike it, it can't directly
		 * be added or subtracted. The timer module provides several
		 * functions to operate on such numbers. Be careful there, as
		 * some functions of libmad's timer module receive some of
		 * their mad_timer_t arguments by value!
		 */

			FrameCount++;
			mad_timer_add(&Timer,Frame.header.duration);

		/* Between the frame decoding and samples synthesis we can
		 * perform some operations on the audio data. We do this only
		 * if some processing was required. Detailed explanations are
		 * given in the ApplyFilter() function.
		 */
		//if(DoFilter) ApplyFilter(&Frame);

		/* Once decoded the frame is synthesized to PCM samples. No errors
		 * are reported by mad_synth_frame();
		 */

			mad_synth_frame(&Synth,&Frame);

		/* Synthesized samples must be converted from libmad's fixed
		 * point number to the consumer format. Here we use unsigned
		 * 16 bit big endian integers on two channels. Integer samples
		 * are temporarily stored in a buffer that is flushed when
		 * full.
		 */

			for(i=0;i<Synth.pcm.length;i++)	{
				signed short	Sample;
				/* Left channel */
				Sample=MadFixedToSshort(Synth.pcm.samples[0][i]);
				*(OutputPtr++)=Sample&0xff;
				*(OutputPtr++)=(Sample>>8);
				/* Right channel. If the decoded stream is monophonic then
			 	* the right output channel is the same as the left one.
			 	*/
				if (MAD_NCHANNELS(&Frame.header) == 2)
					Sample=MadFixedToSshort(Synth.pcm.samples[1][i]);
				*(OutputPtr++)=Sample&0xff;
				*(OutputPtr++)=(Sample>>8);

				/* Flush the output buffer if it is full. */
				if (OutputPtr == OutputBufferEnd) {
					int vol = MAXVOLUME*mp3_volume/100;
					sceAudioOutputPannedBlocking(mp3_handle, vol, vol, (char*)OutputBuffer[OutputBuffer_flip] );
					OutputBuffer_flip^=1;
					OutputPtr=OutputBuffer[OutputBuffer_flip];
					OutputBufferEnd=OutputBuffer[OutputBuffer_flip]+OUTPUT_BUFFER_SIZE;
				}
			}
		}
		if (f >= 0) {
			sceIoClose(f);f=-1;
		}
		//if (!MP3Thread_Active)
			//break;
	}
#ifdef NOEXIT
	MP3Thread_Active = 0;
	MP3_playing = 0;
}
#endif
	// Cleaning up
	return (0);
}

int module_start(SceSize args, void *argp)
{
	/* Create a high priority thread */
	mp3_thread = sceKernelCreateThread("IRSMP3", irsmp3_thread, 0x12, 0x04000, 0, NULL);
	if(mp3_thread >= 0)
	{
		sceKernelStartThread(mp3_thread, args, argp);
	}

	return 0;
}


