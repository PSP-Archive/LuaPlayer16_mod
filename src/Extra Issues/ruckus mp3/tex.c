//
//  Tex
//  Texture manager for PSP
//

#include <tex.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pspgu.h>
#include <pspdebug.h>
#include <rle.h>


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Constants ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


#define TEX_TEXTURE_TABLE_SIZE (256)

#define TEX_FLAG_RLE (0x01)
#define TEX_BAD_FLAG_MASK (0xfe)


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Types --------------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


typedef struct _TexMemoryBlockDescriptor
{
	void                             *base;
	u32                               size;
	struct _TexMemoryBlockDescriptor *next;
	struct _TexMemoryBlockDescriptor *prev;
}
TexMemoryBlockDescriptor;


typedef struct _TexTextureTableEntry
{
	char                         *file;
	u16                           referenceCount;
	TexTexture                   *texture;
	struct _TexTextureTableEntry *next;
	struct _TexTextureTableEntry *prev;
}
TexTextureTableEntry;


#pragma pack(1)

typedef struct
{
	u16 bufferWidth;
	u16 bufferHeight;
	u16 imageWidth;
	u16 imageHeight;
	u8  pixelFormat; // uses GU_PSM_* constants in pspgu.h
	u8  flags;
	u32 pixelBytes;
}
TexFileHeader;

#pragma pack()


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Variables ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


TexMemoryBlockDescriptor *gTexFreeBlocks = NULL;
TexMemoryBlockDescriptor *gTexUsedBlocks = NULL;

TexTextureTableEntry *gTexTextureTable[TEX_TEXTURE_TABLE_SIZE] = { NULL };

u32 gTexBytesPerPixel[4] = { 2, 2, 2, 4 };


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Prototypes ---------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


void *texAllocateVRAM(u32 size);
void texFreeVRAM(void *pointer);

void texAddMemoryBlock(TexMemoryBlockDescriptor *block, TexMemoryBlockDescriptor **list);
void texRemoveMemoryBlock(TexMemoryBlockDescriptor *block, TexMemoryBlockDescriptor **list);

void texMergeFreeBlocks();

int texPowerOfTwo(int i);
int texMultipleOfEight(int i);

u32 texHash(const char *key);


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Functions ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


void texInit(void *vramBlockBase, u32 vramBlockSize)
{
	// Calculate vramBlockSize if necessary.
	
	if (vramBlockSize == TEX_MAX_BLOCK_SIZE)
	{
		u32 base = ((u32) vramBlockBase) & 0x00ffffff;
		vramBlockSize = 0x00200000 - base;
	}
	
	
	// Align buffer on a quadword boundary.
	
	u32 boundaryOffset = 16 - (((u32) vramBlockBase) % 16);
	if (boundaryOffset == 16) boundaryOffset = 0;
	
	vramBlockBase += boundaryOffset;
	vramBlockSize -= boundaryOffset;
	
	
	// Create block lists.
	
	gTexFreeBlocks = malloc(sizeof(TexMemoryBlockDescriptor));
	gTexUsedBlocks = NULL;
	
	if (! gTexFreeBlocks) return;
	
	gTexFreeBlocks->base = vramBlockBase;
	gTexFreeBlocks->size = vramBlockSize;
	gTexFreeBlocks->next = NULL;
	gTexFreeBlocks->prev = NULL;
}


TexTexture *texLoad(const char *file)
{
	// Find texture in texture table.
	
	u32 hash = texHash(file);
	TexTextureTableEntry *entry = gTexTextureTable[hash];
	
	while (entry)
	{
		if (strcmp(entry->file, file) == 0)
		{
			entry->referenceCount++;
			return entry->texture;
		}
		
		entry = entry->next;
	}
	
	
	// Open file.
	
	int fd = open(file, O_RDONLY, 0);
	if (fd < 0) return NULL;
	
	
	// Create texture data structure.
	
	TexTexture *texture = malloc(sizeof(TexTexture));
	if (! texture) goto TEX_LOAD_EXIT;
	
	
	// Read file header.

	TexFileHeader header;
	int bytesRead = read(fd, &header, sizeof(header));
	
	if (bytesRead != sizeof(header) || header.flags & TEX_BAD_FLAG_MASK)
	{
		free(texture);
		texture = NULL;
		goto TEX_LOAD_EXIT;
	}
	
	texture->pixelFormat  = header.pixelFormat;
	texture->bufferWidth  = header.bufferWidth;
	texture->bufferHeight = header.bufferHeight;
	texture->imageWidth   = header.imageWidth;
	texture->imageHeight  = header.imageHeight;
	
	
	// If data is run-length encoded, decode it in main RAM.
	
	unsigned char *decodedPixels = NULL;
	unsigned decodedPixelBytes;
	
	if (header.flags & TEX_FLAG_RLE)
	{
		// Read encoded pixels.
		
		unsigned char *encodedPixels = malloc(header.pixelBytes);
		if (! encodedPixels)
		{
			free(texture);
			texture = NULL;
			goto TEX_LOAD_EXIT;
		}
		
		bytesRead = read(fd, encodedPixels, header.pixelBytes);
		if (header.pixelBytes != bytesRead)
		{
			free(encodedPixels);
			free(texture);
			texture = NULL;
			goto TEX_LOAD_EXIT;
		}
		
		
		// Decode.
		
		unsigned elementSize = *encodedPixels;
		
		rleDecode(encodedPixels + 1, header.pixelBytes - 1, &decodedPixels, &decodedPixelBytes, elementSize);
		free(encodedPixels);
		
		if (! decodedPixels)
		{
			free(texture);
			texture = NULL;
			goto TEX_LOAD_EXIT;
		}
	}
		
	
	// Create pixel buffer.
	
	texture->pixels = texAllocateVRAM(decodedPixels ? decodedPixelBytes : header.pixelBytes);
	
	if (! texture->pixels)
	{
		free(texture);
		texture = NULL;
		goto TEX_LOAD_EXIT;
	}
	
	
	// Read pixels.
	
	if (decodedPixels)
	{
		memcpy(texture->pixels, decodedPixels, decodedPixelBytes);
		free(decodedPixels);
	}
	else
	{
		bytesRead = read(fd, texture->pixels, header.pixelBytes);
		
		if (header.pixelBytes != bytesRead)
		{
			texFreeVRAM(texture->pixels);
			free(texture);
			texture = NULL;
			goto TEX_LOAD_EXIT;
		}
	}
	
	
	// Calculate fractional dimensions.
	
	u16 bufferHeight2 = texPowerOfTwo(texture->imageHeight);
	
	texture->fractionWidth  = ((float) texture->imageWidth)  / ((float) texture->bufferWidth);
	texture->fractionHeight = ((float) texture->imageHeight) / ((float) bufferHeight2);
	
	
	// Add to texture table.
	
	TexTextureTableEntry *newEntry = malloc(sizeof(TexTextureTableEntry));
	if (newEntry) newEntry->file = malloc(strlen(file) + 1);
	
	if (! (newEntry && newEntry->file))
	{
		if (newEntry) free(newEntry);
		texFreeVRAM(texture->pixels);
		free(texture);
		texture = NULL;
		goto TEX_LOAD_EXIT;
	}
	
	strcpy(newEntry->file, file);
	newEntry->referenceCount = 1;
	newEntry->texture = texture;
	newEntry->prev = NULL;
	newEntry->next = NULL;
	
	entry = gTexTextureTable[hash];
	
	if (! entry)
		gTexTextureTable[hash] = newEntry;
	else
	{
		while (entry->next)
			entry = entry->next;
			
		entry->next = newEntry;
	}
	
	newEntry->prev = entry;
	texture->tableEntry = newEntry;

	
	// Clean up.
	
TEX_LOAD_EXIT:
	close(fd);
	return texture;
}


TexTexture *texNew(u16 width, u16 height, u16 pixelFormat)
{
	// Create texture structure.
	
	TexTexture *texture = malloc(sizeof(TexTexture));
	
	texture->pixelFormat  = pixelFormat;
	texture->bufferWidth  = texPowerOfTwo(width);
	texture->bufferHeight = texMultipleOfEight(height);
	texture->imageWidth   = width;
	texture->imageHeight  = height;
	
	texture->pixels = texAllocateVRAM(texture->bufferWidth * texture->bufferHeight * (pixelFormat == GU_PSM_8888 ? 4 : 2));
	if (! texture->pixels)
	{
		free(texture);
		return NULL;
	}
	
	
	// Add to texture table.
	
	TexTextureTableEntry *newEntry = malloc(sizeof(TexTextureTableEntry));
	if (newEntry) newEntry->file = malloc(1);
	
	if (! (newEntry && newEntry->file))
	{
		if (newEntry) free(newEntry);
		texFreeVRAM(texture->pixels);
		free(texture);
		return NULL;
	}
	
	newEntry->file[0] = 0;
	newEntry->referenceCount = 1;
	newEntry->texture = texture;
	newEntry->prev = NULL;
	newEntry->next = NULL;
	
	u32 hash = texHash("");
	TexTextureTableEntry *entry = gTexTextureTable[hash];
	
	if (! entry)
		gTexTextureTable[hash] = newEntry;
	else
	{
		while (entry->next)
			entry = entry->next;
			
		entry->next = newEntry;
	}
	
	newEntry->prev = entry;
	texture->tableEntry = newEntry;
	
	
	return texture;
}


void texSwizzle(TexTexture *texture)
{
	u8* out;
	u8* in;
	unsigned width;
	unsigned height;
	
	out = malloc(texture->bufferWidth * texture->bufferHeight * (texture->pixelFormat == GU_PSM_8888 ? 4 : 2));
	if (! out) return;
	
	in = texture->pixels;
	width = texture->bufferWidth * (texture->pixelFormat == GU_PSM_8888 ? 4 : 2);
	height = texture->bufferHeight;
	
	
	unsigned int blockx, blocky;
	unsigned int j;
	
	unsigned int width_blocks = (width / 16);
	unsigned int height_blocks = (height / 8);
	
	unsigned int src_pitch = (width-16)/4;
	unsigned int src_row = width * 8;
	
	const u8* ysrc = in;
	u32* dst = (u32*) out;
	
	for (blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*) xsrc;
			for (j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}


void texFree(TexTexture *texture)
{
	TexTextureTableEntry *entry = (TexTextureTableEntry *) texture->tableEntry;
	entry->referenceCount--;
	
	if (entry->referenceCount == 0)
	{
		if (entry->next) entry->next->prev = entry->prev;
		if (entry->prev)
			entry->prev->next = entry->next;
		else
		{
			u32 hash = texHash(entry->file);
			gTexTextureTable[hash] = entry->next;
		}
		
		free(entry->file);
		free(entry);
		
		texFreeVRAM(texture->pixels);
		free(texture);
	}
}


TexTexture **texLoadList(const char **files, unsigned count)
{
	if (count == 0) return NULL;
	
	TexTexture **textures = malloc(count * sizeof(TexTexture*));
	if (! textures) return NULL;
	
	while (count--)
		textures[count] = texLoad(files[count]);
	
	return textures;
}


void texFreeList(TexTexture **textures, unsigned count)
{
	if (count == 0) return;
	
	while (count--) texFree(textures[count]);
	free(textures);
}


void texSet(const TexTexture *texture)
{
	sceGuTexMode(texture->pixelFormat, 0, 0, 1);
	sceGuTexImage(0, texture->bufferWidth, texture->bufferHeight, texture->bufferWidth, texture->pixels);
}


void *texAllocateVRAM(u32 size)
{
	if (size == 0) return NULL;
	
	TexMemoryBlockDescriptor *block;
	TexMemoryBlockDescriptor *bestBlock = gTexFreeBlocks;
	
	
	// Find best-fit block.
	
	block = gTexFreeBlocks;
	while (block)
	{
		if (block->size >= size && block->size < bestBlock->size)
			bestBlock = block;
		
		block = block->next;
	}
	
	if ((! bestBlock) || bestBlock->size < size)
		return NULL;
	
	
	// Move free block to used block list.
	
	if (bestBlock->size == size)
	{
		texRemoveMemoryBlock(bestBlock, &gTexFreeBlocks);
		texAddMemoryBlock(bestBlock, &gTexUsedBlocks);
	}
	else
	{
		TexMemoryBlockDescriptor *newFreeBlock = malloc(sizeof(TexMemoryBlockDescriptor));
		if (! newFreeBlock) return NULL;
		
		newFreeBlock->base = (void *) (((u32) bestBlock->base) + size);
		newFreeBlock->size = bestBlock->size - size;
		newFreeBlock->next = NULL;
		newFreeBlock->prev = NULL;
		
		bestBlock->size = size;
		
		texRemoveMemoryBlock(bestBlock, &gTexFreeBlocks);
		texAddMemoryBlock(bestBlock, &gTexUsedBlocks);
		texAddMemoryBlock(newFreeBlock, &gTexFreeBlocks);
	}
	
	texMergeFreeBlocks();
	
	
	return bestBlock->base;
}


void texFreeVRAM(void *pointer)
{
	TexMemoryBlockDescriptor *block;
	
	
	// Find pointer in used block list.
	
	block = gTexUsedBlocks;
	while (block)
	{
		if (block->base == pointer) break;
		block = block->next;
	}
	
	if (! block) return;
	
	
	// Move block from used list to free list.
	
	texRemoveMemoryBlock(block, &gTexUsedBlocks);
	texAddMemoryBlock(block, &gTexFreeBlocks);
	texMergeFreeBlocks();
}


void texAddMemoryBlock(TexMemoryBlockDescriptor *block, TexMemoryBlockDescriptor **list)
{
	if (! *list)
	{
		*list = block;
		return;
	}
	
	
	// Insert block in sort order by base address.
	
	TexMemoryBlockDescriptor *listBlock = *list;
	
	while (listBlock)
	{
		if (listBlock->base > block->base)
		{
			if (listBlock == *list)
			{
				*list = block;
				block->next = listBlock;
				listBlock->prev = block;
			}
			else
			{
				block->prev = listBlock->prev;
				block->next = listBlock;
				
				listBlock->prev->next = block;
				listBlock->prev = block;
			}
			
			break;
		}
		else if (listBlock->next == NULL)
		{
			block->prev = listBlock;
			listBlock->next = block;
			
			break;
		}
		
		listBlock = listBlock->next;
	}
}


void texRemoveMemoryBlock(TexMemoryBlockDescriptor *block, TexMemoryBlockDescriptor **list)
{
	if (*list == block)
		*list = block->next;
	
	if (block->prev != NULL)
		block->prev->next = block->next;
	
	if (block->next != NULL)
		block->next->prev = block->prev;
	
	block->prev = NULL;
	block->next = NULL;
}


void texMergeFreeBlocks()
{
	TexMemoryBlockDescriptor *block;
	
	
	// Find adjacent free blocks.
	
	block = gTexFreeBlocks;
	while (block && block->next)
	{
		TexMemoryBlockDescriptor *nextBlock = block->next;
		
		if (((u32) block->base) + block->size == (u32) nextBlock->base)
		{
			block->size += nextBlock->size;
			block->next = nextBlock->next;
			
			if (block->next)
				block->next->prev = block;
			
			free(nextBlock);
			
			continue;
		}
		
		block = block->next;
	}
}


int texPowerOfTwo(int i)
{
	if (i <=   2) return   2;
	if (i <=   4) return   4;
	if (i <=   8) return   8;
	if (i <=  16) return  16;
	if (i <=  32) return  32;
	if (i <=  64) return  64;
	if (i <= 128) return 128;
	if (i <= 256) return 256;
	
	return 512;
}


int texMultipleOfEight(int i)
{
	int multiple = i / 8;
	if (i % 8) multiple++;
	return multiple;
}


u32 texHash(const char *key)
{
	unsigned length = strlen(key);
	u32 hash = length;
	
	while (length--)
		hash = ((hash << 5) ^ (hash >> 27)) ^ *key++;
	
	return hash % TEX_TEXTURE_TABLE_SIZE;
}
