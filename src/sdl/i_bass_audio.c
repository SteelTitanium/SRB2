#include "bass.h"
#include "../i_sound.h"

UINT8 sound_started = false;
static boolean songpaused;
static UINT8 music_volume, sfx_volume, internal_volume;
static const char* BassErrorCodeToString(int bass_errorcode);
HSTREAM music;

static UINT8 music_volume, sfx_volume, internal_volume;
static float loop_point;
static float song_length; // length in seconds
static boolean songpaused;
static UINT32 music_bytes;
static boolean is_looping;
// fading
static boolean is_fading;
static UINT8 fading_source;
static UINT8 fading_target;
static UINT32 fading_timer;
static UINT32 fading_duration;
//static INT32 fading_id;
static void (*fading_callback)(void);
static boolean fading_nocleanup;

const char *bec2str[] = {
	"All is OK",
	"Memory error",
	"Can't open the file",
	"Can't find a free/valid driver",
	"The sample buffer was lost",
	"Invalid handle",
	"Unsupported sample format",
	"Invalid position",
	"BASS_Init has not been successfully called",
	"BASS_Start has not been successfully called",
	"Already initialized/paused/whatever",
	"Not paused",
	"Not an audio track",
	"Can't get a free channel",
	"An illegal type was specified",
	"An illegal parameter was specified",
	"No 3D support",
	"No EAX support",
	"Illegal device number",
	"Not playing",
	"Illegal sample rate",
	"The stream is not a file stream",
	"No hardware voices available",
	"The MOD music has no sequence data",
	"No internet connection could be opened",
	"Couldn't create the file",
	"Effects are not available",
	"The channel is playing",
	"Requested data is not available",
	"The channel is a 'decoding channel",
	"A sufficient DirectX version is not installed",
	"Connection timedout",
	"Unsupported file format",
	"Unavailable speaker",
	"Invalid BASS version",
	"Codec is not available/supported",
	"The channel/file has ended",
	"The device is busy"
};

static void var_cleanup(void)
{
	song_length = loop_point = 0.0f;
	music_bytes = fading_source = fading_target =\
	 fading_timer = fading_duration = 0;

	songpaused = is_looping =\
	 is_fading = false;

	// HACK: See music_loop, where we want the fade timing to proceed after a non-looping
	// song has stopped playing
	if (!fading_nocleanup)
		fading_callback = NULL;
	else
		fading_nocleanup = false; // use it once, set it back immediately

	internal_volume = 100;
}

static const char* BassErrorCodeToString(int bass_errorcode)
{
	if (bass_errorcode == BASS_ERROR_UNKNOWN) // Special case
		return "unknown";
	return bec2str[bass_errorcode];
}

void *I_GetSfx(sfxinfo_t *sfx)
{
	(void)sfx;
	return NULL;
}

void I_FreeSfx(sfxinfo_t *sfx)
{
	(void)sfx;
}

void I_StartupSound(void)
{
	static long bassver;
	//I_Assert(!sound_started);
	if (sound_started)
		return;

	(void)internal_volume;
	(void)music_volume;
	(void)sfx_volume;

	bassver = BASS_GetVersion();

	CONS_Printf("Compiled with BASS version: %s\n", BASSVERSIONTEXT);
	CONS_Printf("Linked with BASS version: %ld.%ld\n", bassver/100/100/100, bassver/100/100%100);

	// Initalize BASS and the output device
	if (!BASS_Init(-1, 44100, BASS_DEVICE_STEREO|BASS_DEVICE_LATENCY|BASS_DEVICE_FREQ, NULL, NULL))
	{
		CONS_Alert(CONS_ERROR, "Error initializing BASS: %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
		// call to start audio failed -- we do not have it
		return;
	}
	else
		CONS_Printf("BASS was successfully initialized\n");

	fading_nocleanup = false;

	var_cleanup();

	music = 0;
	music_volume = sfx_volume = 0;

#if 0
#ifdef HAVE_MIXERX
	Mix_SetMidiPlayer(cv_midiplayer.value);
	Mix_SetSoundFonts(cv_midisoundfontpath.string);
	Mix_Timidity_addToPathList(cv_miditimiditypath.string);
#endif
#if SDL_MIXER_VERSION_ATLEAST(1,2,11)
	Mix_Init(MIX_INIT_FLAC|MIX_INIT_MP3|MIX_INIT_OGG|MIX_INIT_MOD);
#endif

	if (Mix_OpenAudio(SAMPLERATE, AUDIO_S16SYS, 2, BUFFERSIZE) < 0)
	{
		CONS_Alert(CONS_ERROR, "Error starting SDL_Mixer: %s\n", Mix_GetError());
		// call to start audio failed -- we do not have it
		return;
	}

#ifdef HAVE_OPENMPT
	CONS_Printf("libopenmpt version: %s\n", openmpt_get_string("library_version"));
	CONS_Printf("libopenmpt build date: %s\n", openmpt_get_string("build"));
#endif

	sound_started = true;
	songpaused = false;
	Mix_AllocateChannels(256);
#endif
	sound_started = true;
	songpaused = false;
}

void I_ShutdownSound(void)
{
	if (!sound_started)
		return; // not an error condition
	sound_started = false;

	if (!BASS_Free())
		CONS_Alert(CONS_ERROR, "BASS_Free() failed... %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
}

void I_UpdateSound(void){};

//
//  SFX I/O
//

INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel)
{
	(void)id;
	(void)vol;
	(void)sep;
	(void)pitch;
	(void)priority;
	(void)channel;
	return -1;
}

void I_StopSound(INT32 handle)
{
	(void)handle;
}

boolean I_SoundIsPlaying(INT32 handle)
{
	(void)handle;
	return false;
}

void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{
	(void)handle;
	(void)vol;
	(void)sep;
	(void)pitch;
}

void I_SetSfxVolume(UINT8 volume)
{
	(void)volume;
}

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

void I_InitMusic(void){}

void I_ShutdownMusic(void){}

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

musictype_t I_SongType(void)
{
	return MU_NONE;
}

boolean I_SongPlaying(void)
{
	return false;
}

boolean I_SongPaused(void)
{
	return false;
}

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

boolean I_SetSongSpeed(float speed)
{
	(void)speed;
	return false;
}

/// ------------------------
//  MUSIC SEEKING
/// ------------------------

UINT32 I_GetSongLength(void)
{
	return 0;
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
        (void)looppoint;
        return false;
}

UINT32 I_GetSongLoopPoint(void)
{
	return 0;
}

boolean I_SetSongPosition(UINT32 position)
{
    (void)position;
    return false;
}

UINT32 I_GetSongPosition(void)
{
    return 0;
}

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

boolean I_PlaySong(boolean looping)
{
	is_looping = looping;
	if (!BASS_ChannelPlay(music, looping))
	{
		CONS_Alert(CONS_ERROR, "BASS_ChannelPlay: %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
		return false;
	}
	return true;
}

void I_StopSong(void)
{
	if (music)
		BASS_ChannelStop(music);
}

void I_PauseSong(void)
{
	if (!BASS_ChannelPause(music))
		CONS_Alert(CONS_ERROR, "BASS_ChannelPause: %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
	songpaused = true;
}

void I_ResumeSong(void)
{
	if (!BASS_ChannelPlay(music, is_looping))
		CONS_Alert(CONS_ERROR, "BASS_ChannelPlay: %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
	songpaused = false;
}

boolean I_LoadSong(char *data, size_t len)
{
	if (music)
		I_UnloadSong();

	music = BASS_StreamCreateFile(true, data, 0, len, 0);

	if (!music)
	{
		CONS_Alert(CONS_ERROR, "BASS_StreamCreateFile: %s\n", BassErrorCodeToString(BASS_ErrorGetCode()));
		return false;
	}
	return true;
}

void I_UnloadSong(void)
{
	I_StopSong();

	if (music)
		BASS_StreamFree(music);
}

void I_SetMusicVolume(UINT8 volume)
{
	(void)volume;
}

boolean I_SetSongTrack(int track)
{
	(void)track;
	return false;
}

/// ------------------------
//  MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_StopFadingSong(void)
{
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeOutStopSong(UINT32 ms)
{
	(void)ms;
	return false;
}

boolean I_FadeInPlaySong(UINT32 ms, boolean looping)
{
        (void)ms;
        (void)looping;
        return false;
}
