//
//  mp3utils.c
//  Utilities for handling MP3 metadata (ID3 tags) and such
//
//  Licensed under the BSD license. See LICENSE for details.
//  Copyright (c) 2006 Daniel S. Neumeyer <dan@captaindan.org>
//

#include <mp3utils.h>
#include <pspgu.h>
#include <psppower.h>
#include <mad.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Constants ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


#define MP3_ID3_220_UNSYNCH_FLAG      (0x80)
#define MP3_ID3_220_COMPRESSION_FLAG  (0x40)
#define MP3_ID3_220_UNKNOWN_FLAG_MASK (0x3f)

#define MP3_ID3_230_UNSYNCH_FLAG      (0x80)
#define MP3_ID3_230_EXTENDED_HDR_FLAG (0x40)
#define MP3_ID3_230_EXPERIMENTAL_FLAG (0x20)
#define MP3_ID3_230_UNKNOWN_FLAG_MASK (0x1f)

#define MP3_ID3_240_UNSYNCH_FLAG      (0x80)
#define MP3_ID3_240_EXTENDED_HDR_FLAG (0x40)
#define MP3_ID3_240_EXPERIMENTAL_FLAG (0x20)
#define MP3_ID3_240_FOOTER_FLAG       (0x10)
#define MP3_ID3_240_UNKNOWN_FLAG_MASK (0x0f)


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Types --------------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


#pragma pack(1)

typedef struct
{
	char identifier[3];
	u16  version;
	u8   flags;
	u32  tagSize;
}
MP3ID3v220Header;

typedef MP3ID3v220Header MP3ID3v230Header;
typedef MP3ID3v220Header MP3ID3v240Header;

#pragma pack()


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Prototypes ---------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


void mp3GetID3FrameV2(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize);
void mp3GetID3FrameV3(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize);
void mp3GetID3FrameV4(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize);

char *mp3ExtractText(const void *data, u32 size);
void mp3UTF16toASCII(const char *utf16, u32 length, char *ascii);
void mp3UTF16BEtoASCII(const char *utf16, u32 length, char *ascii);
void mp3UTF8toASCII(const char *utf8, u32 length, char *ascii);

s16 mp3ScaleSample(mad_fixed_t sample);


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Functions ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//



MP3ID3Tag *mp3GetID3Tag(const void *mp3Data)
{
	// Verify presence of ID3v2 tag.
	
	char identifier[4];
	
	memcpy(identifier, mp3Data, 3);
	identifier[3] = 0;
	
	if (strcmp(identifier, "ID3") != 0)
		return NULL;
	
	
	// Verify that tag version is supported.
	
	u8 majorVersion = ((const char *) mp3Data)[3];
	
	if (majorVersion < 2 || majorVersion > 4)
		return NULL;
	
	
	// Verify that tag flags are supported.
	
	u8 flags;
	
	switch (majorVersion)
	{
		case 2:
			flags = ((const char *) mp3Data)[5];
			if (flags)
				return NULL;
			break;
		
		case 3:
			flags = ((const char *) mp3Data)[5];
			if (flags & ~MP3_ID3_230_EXTENDED_HDR_FLAG)
				return NULL;
			break;
		
		case 4:
			flags = ((const char *) mp3Data)[5];
			if (flags & ~(MP3_ID3_240_EXTENDED_HDR_FLAG | MP3_ID3_240_FOOTER_FLAG))
				return NULL;
			break;
	}
	
	
	return (void *) mp3Data;
}


u32 mp3GetID3TagSize(const MP3ID3Tag *id3Tag)
{
	// All supported versions put the size in the same place and in the same format.
	
	MP3ID3v220Header *header = (MP3ID3v220Header *) id3Tag;
	u32 tagSize;
	
	tagSize = ntohl(header->tagSize);
	tagSize = (tagSize & 0x0000007f)
		| ((tagSize & 0x00007f00) >> 1)
		| ((tagSize & 0x007f0000) >> 2)
		| ((tagSize & 0x7f000000) >> 3);
	tagSize += sizeof(MP3ID3v220Header);
	
	return tagSize;
}


char *mp3GetTitle(const MP3ID3Tag *id3Tag)
{
	// Get raw frame data.
	
	void *data;
	u32 size;
	
	u8 majorVersion = ((const char *) id3Tag)[3];
	switch (majorVersion)
	{
		case 2:
			mp3GetID3FrameV2(id3Tag, "TT2", 0, &data, &size);
			break;
		
		case 3:
			mp3GetID3FrameV3(id3Tag, "TIT2", 0, &data, &size);
			break;
		
		case 4:
			mp3GetID3FrameV4(id3Tag, "TIT2", 0, &data, &size);
			break;
	}
	
	
	// Extract title.
	
	char *text = NULL;
	
	if (data)
	{
		text = mp3ExtractText(data, size);
		free(data);
	}
	
	return text;
}


char *mp3GetAlbum(const MP3ID3Tag *id3Tag)
{
	// Get raw frame data.
	
	void *data;
	u32 size;
	
	u8 majorVersion = ((const char *) id3Tag)[3];
	switch (majorVersion)
	{
		case 2:
			mp3GetID3FrameV2(id3Tag, "TAL", 0, &data, &size);
			break;
		
		case 3:
			mp3GetID3FrameV3(id3Tag, "TALB", 0, &data, &size);
			break;
		
		case 4:
			mp3GetID3FrameV4(id3Tag, "TALB", 0, &data, &size);
			break;
	}
	
	
	// Extract album.
	
	char *text = NULL;
	
	if (data)
	{
		text = mp3ExtractText(data, size);
		free(data);
	}
	
	return text;
}


char *mp3GetArtist(const MP3ID3Tag *id3Tag)
{
	// Get raw frame data.
	
	void *data;
	u32 size;
	
	u8 majorVersion = ((const char *) id3Tag)[3];
	switch (majorVersion)
	{
		case 2:
			mp3GetID3FrameV2(id3Tag, "TP1", 0, &data, &size);
			break;
		
		case 3:
			mp3GetID3FrameV3(id3Tag, "TPE1", 0, &data, &size);
			break;
		
		case 4:
			mp3GetID3FrameV4(id3Tag, "TPE1", 0, &data, &size);
			break;
	}
	
	
	// Extract artist.
	
	char *text = NULL;
	
	if (data)
	{
		text = mp3ExtractText(data, size);
		free(data);
	}
	
	return text;
}


int mp3GetTempo(const MP3ID3Tag *id3Tag)
{
	// Get raw frame data.
	
	void *data;
	u32 size;
	
	u8 majorVersion = ((const char *) id3Tag)[3];
	switch (majorVersion)
	{
		case 2:
			mp3GetID3FrameV2(id3Tag, "TBP", 0, &data, &size);
			break;
		
		case 3:
			mp3GetID3FrameV3(id3Tag, "TPBM", 0, &data, &size);
			break;
		
		case 4:
			mp3GetID3FrameV4(id3Tag, "TPBM", 0, &data, &size);
			break;
	}
	
	
	// Extract tempo.
	
	char *text = NULL;
	int tempo = 0;
	
	if (data)
	{
		text = mp3ExtractText(data, size);
		free(data);
		
		sscanf(text, "%d", &tempo);
		free(text);
	}
	
	return tempo;
}


void *mp3GetPrivateData(const MP3ID3Tag *id3Tag, const char *url, u32 *outLength)
{
	unsigned index = 0;
	void *data;
	u32 size;
	
	u8 majorVersion = ((const char *) id3Tag)[3];
	
	while (1)
	{
		switch (majorVersion)
		{
			case 2:
				mp3GetID3FrameV2(id3Tag, "PRV", index, &data, &size);
				break;
			
			case 3:
				mp3GetID3FrameV3(id3Tag, "PRIV", index, &data, &size);
				break;
			
			case 4:
				mp3GetID3FrameV4(id3Tag, "PRIV", index, &data, &size);
				break;
		}
		
		if (! data)
			break;
		
		if (strcmp(url, (char *) data) == 0)
			break;
		
		free(data);
		index++;
	}
	
	if (outLength) *outLength = size;
	return data;
}


// void mp3SetPrivateData(void *mp3Data, u32 mp3Length, const char *url, const void *data, u32 dataLength)
// {
// 	
// }


MP3SeekTable *mp3CreateSeekTable(const void *mp3Data, u32 length)
{
	// Initialize table.
	
	MP3SeekTable *seekTable = malloc(sizeof(MP3SeekTable));
	if (! seekTable) return NULL;
	
	u32 seekTableCapacity = 5000;
	seekTable->table = malloc(sizeof(MP3SeekTableEntry) * seekTableCapacity);
	if (! seekTable->table)
	{
		free(seekTable->table);
		free(seekTable);
		return NULL;
	}
	
	seekTable->sampleCount = 0;
	seekTable->entryCount  = 0;
	
	
	// Decode all frame headers, calculate duration, and fill out seek table.
	
	struct mad_stream stream;
	struct mad_header header;
	mad_timer_t       timer;
	
	mad_stream_init(&stream);
	mad_header_init(&header);
	mad_timer_reset(&timer);
	
	for (;;)
	{
		mad_stream_buffer(&stream, mp3Data, length);
		stream.error = 0;
		mad_header_decode(&header, &stream);
		
		if (stream.error)
		{
			if (MAD_RECOVERABLE(stream.error))
			{
				length -= (((u32) stream.next_frame) - ((u32) mp3Data));
				mp3Data = (void *) stream.next_frame;
				continue;
			}
			else if (stream.error == MAD_ERROR_BUFLEN)
			{
				break;
			}
			else
			{
				free(seekTable->table);
				free(seekTable);
				return NULL;
			}
		}
		
		
		if (seekTable->entryCount == seekTableCapacity)
		{
			seekTableCapacity += 5000;
			void *newTable = realloc(seekTable->table, sizeof(MP3SeekTableEntry) * seekTableCapacity);
			
			if (! newTable)
			{
				free(seekTable->table);
				free(seekTable);
				return NULL;
			}
			
			seekTable->table = newTable;
		}
		
		seekTable->table[seekTable->entryCount].sampleOffset = mad_timer_count(timer, MAD_UNITS_44100_HZ);
		seekTable->table[seekTable->entryCount].bufferStart  = (void *) mp3Data;
		seekTable->table[seekTable->entryCount].bufferLength = length;
		
		seekTable->entryCount++;
		
		
		mad_timer_add(&timer, header.duration);
		
		if (stream.next_frame)
		{
			length -= (((u32) stream.next_frame) - ((u32) mp3Data));
			mp3Data = (void *) stream.next_frame;
		}
	}
	
	mad_header_finish(&frame);
	mad_stream_finish(&stream);
	
	
	// Shorten seek table if it's not full.
	
	if (seekTable->entryCount < seekTableCapacity)
		seekTable->table = realloc(seekTable->table, sizeof(MP3SeekTableEntry) * seekTable->entryCount);
	
	
	// Note duration of audio.
	
	seekTable->sampleCount = mad_timer_count(timer, MAD_UNITS_44100_HZ);
	
	return seekTable;
}


void mp3FreeSeekTable(MP3SeekTable *seekTable)
{
	free(seekTable->table);
	free(seekTable);
}


MP3SeekTableEntry *mp3Seek(const MP3SeekTable *seekTable, u32 sampleIndex)
{
	u32 i = seekTable->entryCount;
	while (i--)
	{
		if (seekTable->table[i].sampleOffset <= sampleIndex)
			return &seekTable->table[i];
	}
	
	return NULL; // never gets here, but this shuts up a compiler warning
}


TexTexture *mp3CreateWaveformTexture(const void *mp3Data, u32 length, u16 texWidth, u16 texHeight, MP3ProgressCallback *progressCallback)
{
	// Create texture.
	
	TexTexture *texture = texNew(texWidth, texHeight, GU_PSM_4444);
	if (! texture) return NULL;
	
	u16 *pixels = texture->pixels;
	bzero(pixels, texture->bufferWidth * texture->bufferHeight * sizeof(u16));
	
	
	// Calculate duration.
	
	MP3SeekTable *seekTable = mp3CreateSeekTable(mp3Data, length);
	if (! seekTable)
	{
		texFree(texture);
		return NULL;
	}
	
	u32 duration = seekTable->sampleCount;
	mp3FreeSeekTable(seekTable);
	
	
	// Get a little boost by upclocking the CPU.
	
	int oldCPUFrequency = scePowerGetCpuClockFrequency();
	scePowerSetCpuClockFrequency(333);
	
	
	// Decode frames and draw waveform.
	
	int framesPerPixel   = duration / 1152 / texWidth;
	int frameCounter     = 0;
	void *mp3DataPtr     = (void *) mp3Data;
	u32 mp3DataRemaining = length;
	int x                = 0;
	
	struct mad_stream stream;
	struct mad_header header;
	struct mad_frame  frame;
	struct mad_synth  synth;
	
	mad_stream_init(&stream);
	mad_header_init(&header);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	
	while (1)
	{
		// Decode frame.
		
		mad_stream_buffer(&stream, (void *) mp3DataPtr, mp3DataRemaining);
		
		int error;
		if (frameCounter == 0)
			error = mad_frame_decode(&frame, &stream);
		else
			error = mad_header_decode(&header, &stream);
		
		if (error)
		{
			if (MAD_RECOVERABLE(stream.error))
			{
				if (stream.next_frame)
				{
					mp3DataRemaining -= (((u32) stream.next_frame) - ((u32) mp3DataPtr));
					mp3DataPtr = (void *) stream.next_frame;
				}
				
				continue;
			}
			else
				break;
		}
		
		
		// If enough frames have been skipped, synthesize this frame.
		
		if (frameCounter == 0)
		{
			mad_synth_frame(&synth, &frame);
			
			
			// Find max amplitude of this frame.
			
			s32 maxAmplitude = 0;
			mad_fixed_t *left  = synth.pcm.samples[0];
			mad_fixed_t *right = synth.pcm.samples[1];
			int sampleCount    = synth.pcm.length;
			
			while (sampleCount--)
			{
				s32 thisAmplitude = abs((s32) mp3ScaleSample(*left++));
				if (thisAmplitude > maxAmplitude) maxAmplitude = thisAmplitude;
				
				thisAmplitude = abs((s32) mp3ScaleSample(*right++));
				if (thisAmplitude > maxAmplitude) maxAmplitude = thisAmplitude;
			}
			
			
			// Draw waveform segment.
			
			int segmentHeight = texHeight * maxAmplitude / 32768;
			int startY = (texHeight - segmentHeight) / 2;
			int y;
			
			for (y = startY; y < segmentHeight + startY; y++)
			{
				pixels[y * texture->bufferWidth + x] = 0xf000;
			}
			
			x++;
			
			
			// Call progress callback.
			
			if (progressCallback)
				progressCallback(x, texWidth);
		}
			
		// Advance to next frame.
		
		if (stream.next_frame)
		{
			mp3DataRemaining -= (((u32) stream.next_frame) - ((u32) mp3DataPtr));
			mp3DataPtr = (void *) stream.next_frame;
		}
		
		frameCounter++;
		if (frameCounter == framesPerPixel)
			frameCounter = 0;
	}
	
	
	// Clean up.
	
	scePowerSetCpuClockFrequency(oldCPUFrequency);
	
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_header_finish(&header);
	mad_stream_finish(&stream);
	
	return texture;
}


void mp3GetID3FrameV2(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize)
{
	// Initialize.
	
	*outData = NULL;
	*outSize = 0;
	
	
	// Get header and decode tag size.
	
	MP3ID3v220Header *header = (MP3ID3v220Header *) tag;
	u32 tagSize;
	
	tagSize = ntohl(header->tagSize);
	tagSize = (tagSize & 0x0000007f)
		| ((tagSize & 0x00007f00) >> 1)
		| ((tagSize & 0x007f0000) >> 2)
		| ((tagSize & 0x7f000000) >> 3);
	tagSize += sizeof(MP3ID3v220Header);
	
	
	// Find frame.
	
	u8 *u8Tag = (u8 *) tag;
	u32 i = sizeof(MP3ID3v220Header);
	
	while (i < tagSize)
	{
		// Read frame header.
		
		char thisFrameID[4];
		u32 frameSize = 0;
		
		thisFrameID[0] = u8Tag[i++];
		thisFrameID[1] = u8Tag[i++];
		thisFrameID[2] = u8Tag[i++];
		thisFrameID[3] = 0;
		
		((u8 *) &frameSize)[1] = u8Tag[i++];
		((u8 *) &frameSize)[2] = u8Tag[i++];
		((u8 *) &frameSize)[3] = u8Tag[i++];
		frameSize = ntohl(frameSize);
		
		
		// Is this the requested frame?
		
		if (strcmp(frameID, thisFrameID) == 0)
		{
			if (index == 0)
			{
				u8 *data = malloc(frameSize);
				if (! data) return;
				
				*outData = data;
				*outSize = frameSize;
				
				while (frameSize--)
					*data++ = u8Tag[i++];
				
				return;
			}
			
			index--;
		}
		
		
		// Skip to next frame.
		
		i += frameSize;
	}
}


void mp3GetID3FrameV3(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize)
{
	// Initialize.
	
	*outData = NULL;
	*outSize = 0;
	
	
	// Get header and decode tag size.
	
	MP3ID3v230Header *header = (MP3ID3v230Header *) tag;
	u32 tagSize;
	
	tagSize = ntohl(header->tagSize);
	tagSize = (tagSize & 0x0000007f)
		| ((tagSize & 0x00007f00) >> 1)
		| ((tagSize & 0x007f0000) >> 2)
		| ((tagSize & 0x7f000000) >> 3);
	tagSize += sizeof(MP3ID3v230Header);
	
	
	// Find frame.
	
	u8 *u8Tag = (u8 *) tag;
	u32 i = sizeof(MP3ID3v230Header);

	if (header->flags & MP3_ID3_230_EXTENDED_HDR_FLAG)
	{
		u32 extHdrSize;
		((u8 *) &extHdrSize)[0] = u8Tag[i++];
		((u8 *) &extHdrSize)[1] = u8Tag[i++];
		((u8 *) &extHdrSize)[2] = u8Tag[i++];
		((u8 *) &extHdrSize)[3] = u8Tag[i++];
		i += ntohl(extHdrSize);
	}
	
	while (i < tagSize)
	{
		// Read frame header.
		
		char thisFrameID[5];
		u32 frameSize;
		
		thisFrameID[0] = u8Tag[i++];
		thisFrameID[1] = u8Tag[i++];
		thisFrameID[2] = u8Tag[i++];
		thisFrameID[3] = u8Tag[i++];
		thisFrameID[4] = 0;
		
		((u8 *) &frameSize)[0] = u8Tag[i++];
		((u8 *) &frameSize)[1] = u8Tag[i++];
		((u8 *) &frameSize)[2] = u8Tag[i++];
		((u8 *) &frameSize)[3] = u8Tag[i++];
		frameSize = ntohl(frameSize);
		
		i += 2; // flags
		
		
		// Is this the requested frame?
		
		if (strcmp(frameID, thisFrameID) == 0)
		{
			if (index == 0)
			{
				u8 *data = malloc(frameSize);
				if (! data) return;
				
				*outData = data;
				*outSize = frameSize;
				
				while (frameSize--)
					*data++ = u8Tag[i++];
				
				return;
			}
			
			index--;
		}
		
		
		// Skip to next frame.
		
		i += frameSize;
	}
}


void mp3GetID3FrameV4(const MP3ID3Tag *tag, const char *frameID, unsigned index, void **outData, u32 *outSize)
{
	// Initialize.
	
	*outData = NULL;
	*outSize = 0;
	
	
	// Get header and decode tag size.
	
	MP3ID3v240Header *header = (MP3ID3v240Header *) tag;
	u32 tagSize;
	
	tagSize = ntohl(header->tagSize);
	tagSize = (tagSize & 0x0000007f)
		| ((tagSize & 0x00007f00) >> 1)
		| ((tagSize & 0x007f0000) >> 2)
		| ((tagSize & 0x7f000000) >> 3);
	tagSize += sizeof(MP3ID3v240Header);
	
	
	// Find frame.
	
	u8 *u8Tag = (u8 *) tag;
	u32 i = sizeof(MP3ID3v240Header);
	
	if (header->flags & MP3_ID3_240_EXTENDED_HDR_FLAG)
	{
		u32 extHdrSize;
		((u8 *) &extHdrSize)[0] = u8Tag[i++];
		((u8 *) &extHdrSize)[1] = u8Tag[i++];
		((u8 *) &extHdrSize)[2] = u8Tag[i++];
		((u8 *) &extHdrSize)[3] = u8Tag[i++];
		extHdrSize = ntohl(extHdrSize);
		extHdrSize = (extHdrSize & 0x0000007f)
			| ((extHdrSize & 0x00007f00) >> 1)
			| ((extHdrSize & 0x007f0000) >> 2)
			| ((extHdrSize & 0x7f000000) >> 3);
		
		i += (extHdrSize - 4);
	}
	
	while (i < tagSize)
	{
		// Read frame header.
		
		char thisFrameID[5];
		u32 frameSize;
		
		thisFrameID[0] = u8Tag[i++];
		thisFrameID[1] = u8Tag[i++];
		thisFrameID[2] = u8Tag[i++];
		thisFrameID[3] = u8Tag[i++];
		thisFrameID[4] = 0;
		
		((u8 *) &frameSize)[0] = u8Tag[i++];
		((u8 *) &frameSize)[1] = u8Tag[i++];
		((u8 *) &frameSize)[2] = u8Tag[i++];
		((u8 *) &frameSize)[3] = u8Tag[i++];
		frameSize = ntohl(frameSize);
		frameSize = (frameSize & 0x0000007f)
			| ((frameSize & 0x00007f00) >> 1)
			| ((frameSize & 0x007f0000) >> 2)
			| ((frameSize & 0x7f000000) >> 3);
		
		i += 2; // flags
		
		
		// Is this the requested frame?
		
		if (strcmp(frameID, thisFrameID) == 0)
		{
			if (index == 0)
			{
				u8 *data = malloc(frameSize);
				if (! data) return;
				
				*outData = data;
				*outSize = frameSize;
				
				while (frameSize--)
					*data++ = u8Tag[i++];
				
				return;
			}
			
			index--;
		}
		
		
		// Skip to next frame.
		
		i += frameSize;
	}
}


char *mp3ExtractText(const void *data, u32 size)
{
	char *output = malloc(size + 1);
	if (! output) return NULL;
	bzero(output, size + 1);
	
	u8 encoding = *((u8 *) data);
	
	switch (encoding)
	{
		case 0: // ISO-8859-1
			memcpy(output, data + 1, size - 1);
			break;
		
		case 1: // UTF-16
			mp3UTF16toASCII(data, size, output);
			break;
			
		case 2: // UTF-16BE
			mp3UTF16BEtoASCII(data, size, output);
			break;
		
		case 3: // UTF-8
			mp3UTF8toASCII(data, size, output);
			break;
		
		default:
			free(output);
			return NULL;
			break;
	}
		
	return output;
}


void mp3UTF16toASCII(const char *utf16, u32 length, char *ascii)
{
	// Check byte-order.
	
	u16 bom = (((u8) utf16[1]) << 8) | ((u8) utf16[2]);
	if (bom == 0xFEFF)
	{
		mp3UTF16BEtoASCII(utf16, length, ascii);
		return;
	}
	
	
	// Convert little-endian UTF-16.
	
	u32 i, j;
	
	for (i = 3, j = 0; i < length; i += 2)
	{
		u16 ch = ((u8) utf16[i]) | (((u8) utf16[i + 1]) << 8);
		
		 if (ch < 128)
		 	ascii[j++] = ch;
	}
}


void mp3UTF16BEtoASCII(const char *utf16, u32 length, char *ascii)
{
	u32 i, j;
	
	for (i = 1, j = 0; i < length; i += 2)
	{
		u16 ch = (((u8) utf16[i]) << 8) | ((u8) utf16[i + 1]);
		
		 if (ch < 128)
		 	ascii[j++] = ch;
	}
}


void mp3UTF8toASCII(const char *utf8, u32 length, char *ascii)
{
	u32 i, j;
	
	for (i = 1, j = 0; i < length; i++)
	{
		u8 ch = utf8[i];
		if (ch & 0x80) continue;
		
		ascii[j++] = ch;
	}
}


s16 mp3ScaleSample(mad_fixed_t sample)
{
	if (sample >=  MAD_F_ONE) return( SHRT_MAX);
	if (sample <= -MAD_F_ONE) return(-SHRT_MAX);
	
	sample = sample >> (MAD_F_FRACBITS - 15);
	return ((s16) sample);
}
