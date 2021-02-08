//
//  rle.c
//  Basic run-length encoder (and decoder)
//

#include <rle.h>
#include <string.h>
#include <stdlib.h>


#define RLE_MARKER_BYTE ((unsigned char) 0xfe)


unsigned rleWriteRun(void *element, unsigned runLength, unsigned char *ptr, unsigned elementSize);
void rleIncreaseCapacity(unsigned char **buffer, unsigned *capacity);


void rleEncode(const unsigned char *inBytes, unsigned inSize, unsigned char **outBytes, unsigned *outSize, unsigned elementSize)
{
	// Check for zero-length or NULL input.
	
	if (! (inBytes && inSize))
	{
		*outBytes = NULL;
		*outSize = 0;
		return;
	}
	
	
	// Validate element size.
	
	if (elementSize == 0 || inSize % elementSize)
	{
		*outBytes = NULL;
		*outSize = 0;
		return;
	}
	
	
	// Allocate enough space for the worst-case scenario.
	
	*outBytes = malloc(inSize * 2 + 1);
	*outSize = 0;
	
	if (! *outBytes) return;
	
	
	// Encode.
	
	unsigned char *outPtr = *outBytes;
	void *previousElement = malloc(elementSize);
	unsigned runLength = 1;
	
	if (! previousElement)
	{
		*outBytes = NULL;
		*outSize = 0;
		return;
	}
	
	memcpy(previousElement, inBytes, elementSize);
	inBytes += elementSize;
	inSize /= elementSize;
	inSize--;
	
	while (inSize--)
	{
		if (memcmp(inBytes, previousElement, elementSize))
		{
			unsigned runBytes = rleWriteRun(previousElement, runLength, outPtr, elementSize);
			outPtr += runBytes;
			*outSize += runBytes;
			
			memcpy(previousElement, inBytes, elementSize);
			runLength = 1;
		}
		else
			runLength++;
		
		inBytes += elementSize;
	}
	
	*outSize += rleWriteRun(previousElement, runLength, outPtr, elementSize);
	
	
	// Shrink output buffer to fit.
	
	unsigned char *newBuffer = realloc(*outBytes, *outSize);
	if (newBuffer) *outBytes = newBuffer;
}


void rleDecode(const unsigned char *inBytes, unsigned inSize, unsigned char **outBytes, unsigned *outSize, unsigned elementSize)
{
	// Check for zero-length or NULL input.
	
	if (! (inBytes && inSize))
	{
		*outBytes = NULL;
		*outSize = 0;
		return;
	}
	
	
	// Validate element size.
	
	if (elementSize == 0)
	{
		*outBytes = NULL;
		*outSize = 0;
		return;	
	}
	
	
	// Allocate an output buffer twice the size of the input buffer.
	// There's no efficient way to determine the size of the decoded data.
	
	unsigned outputCapacity = inSize * 2;
	
	*outBytes = malloc(outputCapacity);
	*outSize = 0;
	
	if (! *outBytes) return;
	
	
	// Decode.
	
	unsigned char *outPtr = *outBytes;
	
	while (inSize)
	{
		if (*inBytes == RLE_MARKER_BYTE)
		{
			unsigned runLength = *++inBytes;
			inBytes++;
			
			while (*outSize + (runLength * elementSize) > outputCapacity)
			{
				rleIncreaseCapacity(outBytes, &outputCapacity);
				if (! *outBytes)
				{
					*outSize = 0;
					return;
				}
				
				outPtr = *outBytes + *outSize;
			}
			
			while (runLength--)
			{
				memcpy(outPtr, inBytes, elementSize);
				outPtr += elementSize;
				*outSize += elementSize;
			}
			
			inSize -= (2 + elementSize);
		}
		else
		{
			if (*outSize + elementSize > outputCapacity)
			{
				rleIncreaseCapacity(outBytes, &outputCapacity);
				if (! *outBytes)
				{
					*outSize = 0;
					return;
				}
				
				outPtr = *outBytes + *outSize;
			}
			
			memcpy(outPtr, inBytes, elementSize);
			outPtr += elementSize;
			*outSize += elementSize;
			
			inSize -= elementSize;
		}
		
		inBytes += elementSize;
	}
	
	
	// Shrink output buffer to fit.
	
	unsigned char *newBuffer = realloc(*outBytes, *outSize);
	if (newBuffer) *outBytes = newBuffer;
}


unsigned rleWriteRun(void *element, unsigned runLength, unsigned char *ptr, unsigned elementSize)
{
	// Don't encode a run if the encoding is longer than the run (unless the
	// element begins with the run marker, in which case a run must be encoded).
	
	if ((runLength < (2 + elementSize)) && (*((unsigned char *) element) != RLE_MARKER_BYTE))
	{
		unsigned i;
		for (i = 0; i < runLength; i++)
		{
			memcpy(ptr, element, elementSize);
			ptr += elementSize;
		}
		
		return runLength * elementSize;
	}
	
	
	// Write encoded run.
	
	unsigned bytesWritten = 2 + elementSize;
	
	while (runLength > 255)
	{
		*ptr++ = RLE_MARKER_BYTE;
		*ptr++ = 255;
		memcpy(ptr, element, elementSize);
		ptr += elementSize;
		
		runLength -= 255;
		bytesWritten += (2 + elementSize);
	}
	
	*ptr++ = RLE_MARKER_BYTE;
	*ptr++ = runLength;
	memcpy(ptr, element, elementSize);
	
	return bytesWritten;
}


void rleIncreaseCapacity(unsigned char **buffer, unsigned *capacity)
{
	unsigned newCapacity = *capacity * 2;
	if (newCapacity < *capacity)
	{
		free(*buffer);
		*buffer = NULL;
		*capacity = 0;
		return;
	}
	
	void *newBuffer = realloc(*buffer, newCapacity);
	if (! newBuffer)
	{
		free(*buffer);
		*buffer = NULL;
		*capacity = 0;
		return;
	}
	
	*buffer = newBuffer;
	*capacity = newCapacity;
}
