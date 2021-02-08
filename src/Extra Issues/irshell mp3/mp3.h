
#define MP3_STATUS_ACTIVE	1
#define MP3_STATUS_PLAY		2
#define MP3_STATUS_STOP		3
#define MP3_STATUS_PAUSE	4
#define MP3_STATUS_REPEAT_ONE	5
#define MP3_STATUS_REPEAT_ALL	6

// Play Type
#define MP3_PLAYLIST	11
#define MP3_PLAYSINGLE	12

// Play Mode
#define MP3_PLAY	1
#define MP3_REPEAT	2

// Pause Mode
#define MP3_PAUSE	1
#define MP3_RESUME	0

#define DIR_PATHLEN	0
#define MAX_PLAYLIST	0
#define MAXPATH		0

int irsmp3Status(void);
void irsmp3Play(char *file, int playmode, int offset);
void irsmp3Playlist(char *file, char playlist[][DIR_PATHLEN+1], int playlist_cnt, int playlist_cur, int playmode, int offset);
void irsmp3Pause(int pause);
void irsmp3Resume(void);
void irsmp3Suspend(void);
void irsmp3ASuspend(void);
void irsmp3Stop(void);
void irsmp3AStop(void);
void seteos(int seos);
void irsmp3Time(char *dest);
int irsmp3EndOfStream();
void irsmp3Volume(int vol);
int irsmp3Getlist(char **name, char **dir, char **playlist, int *playlist_cnt, int *playlist_cur, int *playmode, int *offset);
void irsmp3Exit(void);
int module_start(SceSize args, void *argp);
