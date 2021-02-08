//Mp3 Stuff!
#ifndef MP3_HPP
#define MP3_HPP

#include <mad.h>
    void MP3_init(int channel);
    int MP3_play();
    void MP3_stop();
    void MP3_load(char *mp3file);
	//void MP3_pause();
	int MP3_EndOfStream();
#endif
