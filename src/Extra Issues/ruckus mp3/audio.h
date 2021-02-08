//
//  audio.h
//  Where all the real work is done
//
//  Licensed under the BSD license. See LICENSE for details.
//  Copyright (c) 2006 Daniel S. Neumeyer <dan@captaindan.org>
//

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <psptypes.h>


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Constants ----------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


#define RKS_AUDIO_RATE_UNITY (10000)


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// --- Prototypes ---------------------------------------------------------- //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//


void rksAudioInit(int channel);

void rksAudioLoad(const char *file);

u32 rksAudioDuration();
u32 rksAudioPosition();
void rksAudioSeek(u32 frameIndex);

void rksAudioGetInfo(char **outTitle, char **outArtist, char **outAlbum, int *outTempo);

void rksAudioPlay();
void rksAudioPause();
void rksAudioStop();
SceBool rksAudioIsPlaying();
SceBool rksAudioIsPaused();

void rksAudioSetPlaybackRate(u32 fixedRate);


#endif // __AUDIO_H__
