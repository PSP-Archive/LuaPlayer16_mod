#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>

#include <pspaudiolib.h>
#include <pspkernel.h>

extern "C" {
#include "mad.h"
}
#include "mp3.hpp"

#define FALSE 0
#define TRUE !FALSE

      struct Sample
      {
         short left;
         short right;
      };

      static SceUID file = 0;
      static unsigned int fileSize = 1;
      static unsigned int filePos = 0;
      static mad_stream stream;
      static mad_frame frame;
      static mad_synth synth;
      static unsigned char fileBuffer[2048];
      static unsigned int samplesRead;
	  char *mfile;
	  static int isplaying;
	  static int eos;
	  static int myChannel;

      static void fillFileBuffer()
      {
         // Open the file if it's not open.
         if (file <= 0)
         {
            char   cd[1024];
            memset(cd, 0, sizeof(cd));
            getcwd(cd, sizeof(cd) - 1);

            char   fileName[1024];
            memset(fileName, 0, sizeof(fileName));
            snprintf(fileName, sizeof(fileName) - 1, "%s/%s", cd, mfile);

            //pspDebugScreenPrintf("Opening %s... ", fileName);
            file = sceIoOpen(fileName, PSP_O_RDONLY, 777);
            if (file <= 0)
            {
               //pspDebugScreenPrintf("Failed (%d).\n", file);
               return;
            }
            else
            {
               //pspDebugScreenPrintf("OK (%d).\n", file);
            }

            // Get the size.
            fileSize = sceIoLseek(file, 0, SEEK_END);
            sceIoLseek(file, 0, SEEK_SET);
         }

         // Find out how much to keep and how much to fill.
         const unsigned int   bytesToKeep   = stream.bufend - stream.next_frame;
         unsigned int      bytesToFill   = sizeof(fileBuffer) - bytesToKeep;
         //pspDebugScreenPrintf("bytesToFill = %u, bytesToKeep = %u.\n", bytesToFill, bytesToKeep);

         // Want to keep any bytes?
         if (bytesToKeep)
         {
            // Copy the tail to the head.
            memmove(fileBuffer, fileBuffer + sizeof(fileBuffer) - bytesToKeep, bytesToKeep);
         }

         // Read into the rest of the file buffer.
         unsigned char* bufferPos = fileBuffer + bytesToKeep;
         while (bytesToFill > 0)
         {
            // Read some.
            //pspDebugScreenPrintf("Reading %u bytes...\n", bytesToFill);
            const unsigned int bytesRead = sceIoRead(file, bufferPos, bytesToFill);

            // EOF?
            if (bytesRead == 0)
            {
               //pspDebugScreenPrintf("End of file.\n");
               sceIoLseek(file, 0, SEEK_SET);
               filePos = 0;
			   eos = 1;
			   MP3_stop();
               break;
            }

            // Adjust where we're writing to.
            bytesToFill -= bytesRead;
            bufferPos += bytesRead;
            filePos += bytesRead;

            //pspDebugScreenPrintf("Read %u bytes from the file, %u left to fill.\n", bytesRead, bytesToFill);
            //pspDebugScreenPrintf("%u%%.\n", filePos * 100 / fileSize);
         }
      }

      static void decode()
      {
         // While we need to fill the buffer...
         while (
            (mad_frame_decode(&frame, &stream) == -1) &&
            ((stream.error == MAD_ERROR_BUFLEN) || (stream.error == MAD_ERROR_BUFPTR))
            )
         {
            // Fill up the remainder of the file buffer.
            fillFileBuffer();

            // Give new buffer to the stream.
            mad_stream_buffer(&stream, fileBuffer, sizeof(fileBuffer));
         }

         // Synth the frame.
         mad_synth_frame(&synth, &frame);
      }

      static inline short convertSample(mad_fixed_t sample)
      {
         /* round */
         sample += (1L << (MAD_F_FRACBITS - 16));

         /* clip */
         if (sample >= MAD_F_ONE)
            sample = MAD_F_ONE - 1;
         else if (sample < -MAD_F_ONE)
            sample = -MAD_F_ONE;

         /* quantize */
         return sample >> (MAD_F_FRACBITS + 1 - 16);
      }

      static void convertLeftSamples(Sample* first, Sample* last, const mad_fixed_t* src)
      {
         for (Sample* dst = first; dst != last; ++dst)
         {
            dst->left = convertSample(*src++);
         }
      }

      static void convertRightSamples(Sample* first, Sample* last, const mad_fixed_t* src)
      {
         for (Sample* dst = first; dst != last; ++dst)
         {
            dst->right = convertSample(*src++);
         }
      }

      static void fillOutputBuffer(void* buffer, unsigned int samplesToWrite, void* userData)
      {
         // Where are we writing to?
         Sample* destination = static_cast<Sample*> (buffer);

		 if (isplaying == TRUE) {
         // While we've got samples to write...
         while (samplesToWrite > 0)
         {
            // Enough samples available?
            const unsigned int samplesAvailable = synth.pcm.length - samplesRead;
            if (samplesAvailable > samplesToWrite)
            {
               // Write samplesToWrite samples.
               convertLeftSamples(destination, destination + samplesToWrite, &synth.pcm.samples[0][samplesRead]);
               convertRightSamples(destination, destination + samplesToWrite, &synth.pcm.samples[1][samplesRead]);

               // We're still using the same PCM data.
               samplesRead += samplesToWrite;

               // Done.
               samplesToWrite = 0;
            }
            else
            {
               // Write samplesAvailable samples.
               convertLeftSamples(destination, destination + samplesAvailable, &synth.pcm.samples[0][samplesRead]);
               convertRightSamples(destination, destination + samplesAvailable, &synth.pcm.samples[1][samplesRead]);

               // We need more PCM data.
               samplesRead = 0;
               decode();

               // We've still got more to write.
               destination += samplesAvailable;
               samplesToWrite -= samplesAvailable;
            }
         }
		 }
      }

void MP3_init(int channel)
{
	// Initialise the audio system.
	pspAudioInit();
	myChannel = channel;
	isplaying = FALSE;
	pspAudioSetChannelCallback(myChannel, fillOutputBuffer, 0);
	// Set up MAD.
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
}

void MP3_load(char *mp3file)
{
	//Set Which Mp3 to play
	eos = 0;
	mfile = mp3file;
	isplaying = FALSE;
}

void MP3_pause()
{
	isplaying = !isplaying;
}
   
int MP3_play()
{
    if (isplaying)
	return FALSE;

    isplaying = TRUE;
    return TRUE;
}

void MP3_stop()
{
	
	isplaying = FALSE;
	// Shut down audio.
	pspAudioSetChannelCallback(myChannel, 0,0);
	pspAudioEnd();
	
	// Shut down MAD.
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
	
	//Close Audio File
	sceIoClose(file);
}

int MP3_EndOfStream()
{
    return eos;
}
