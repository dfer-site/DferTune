#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

typedef enum {
    TuneShuffleMode_Off,
    TuneShuffleMode_On,

    TuneShuffleMode_Count,
} TuneShuffleMode;

typedef enum {
    TuneRepeatMode_Off,
    TuneRepeatMode_One,
    TuneRepeatMode_All,

    TuneRepeatMode_Count,
} TuneRepeatMode;

typedef enum {
    TuneEnqueueType_Front,
    TuneEnqueueType_Back,

    TuneEnqueueType_Count,
} TuneEnqueueType;

typedef struct {
    u32 sample_rate;
    u32 current_frame;
    u32 total_frames;
} TuneCurrentStats;

/**
 * Runtime policy for startup playback and system UI contexts.
 * Each field is 0 (disabled) or 1 (enabled). This packed snapshot is sent
 * atomically to the sysmodule, avoiding stale per-process config caches.
 */
typedef struct {
    u8 auto_play_startup;
    u8 wait_for_home;
    u8 pause_on_keyboard;
    u8 pause_on_controller_sync;
    u8 pause_on_lockscreen;
    u8 reserved[3];
} TuneStartupPolicy;

#define TUNE_EQUALIZER_BAND_COUNT 5
#define TUNE_EQUALIZER_MIN_GAIN_DB (-12)
#define TUNE_EQUALIZER_MAX_GAIN_DB 12
#define TUNE_EQUALIZER_TARGET_MUSIC 0
#define TUNE_EQUALIZER_TARGET_SYSTEM 1

/** Five-band EQ snapshot. Target selects DferTune music or system/game output. */
typedef struct {
    u8 enabled;
    u8 target;
    s8 gains_db[TUNE_EQUALIZER_BAND_COUNT];
    u8 reserved;
} TuneEqualizerSettings;

Result tuneInitialize();

void tuneExit();

/**
 * @brief Get the current status of playback.
 * @param[out] status \ref AudioOutState
 */
Result tuneGetStatus(bool *status);

Result tunePlay();
Result tunePause();
Result tuneNext();
Result tunePrev();

/**
 * @brief Get the current playback volume.
 * @note On FW lower than [6.0.0] this will set the decode volume.
 * @param[out] out volume value (linear factor).
 */
Result tuneGetVolume(float *out);

/**
 * @brief Set the playback volume.
 * @note On FW lower than [6.0.0] this will return the decode volume.
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetVolume(float volume);

/**
 * @brief Get the volume of the current title
 * @param[out] out volume value (linear factor).
 */
Result tuneGetTitleVolume(float *out);

/**
 * @brief Set the volume of the current title
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetTitleVolume(float volume);

/**
 * @brief Get the default volume of all titles
 * @param[out] out volume value (linear factor).
 */
Result tuneGetDefaultTitleVolume(float *out);

/**
 * @brief Set the default volume of all titles
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetDefaultTitleVolume(float volume);

/**
 * @brief Get the current loop status.
 * @param[out] state \ref TuneRepeatMode
 */
Result tuneGetRepeatMode(TuneRepeatMode *state);

/**
 * @brief Set repeat mode.
 * @param[in] state \ref TuneRepeatMode
 */
Result tuneSetRepeatMode(TuneRepeatMode state);

Result tuneGetShuffleMode(TuneShuffleMode *state);
Result tuneSetShuffleMode(TuneShuffleMode state);

/**
 * @brief Get the current queue size.
 * @param[out] count remaining tracks after current.
 */
Result tuneGetPlaylistSize(u32 *count);

/**
 * @brief Read queue.
 * @param[out] read Amount written to buffer.
 * @param[out] out_path Path array FS_MAX_PATH * n
 * @param[in] out_path_length Size of the supplied path array.
 */
Result tuneGetPlaylistItem(u32 index, char *out_path, size_t out_path_length);

/**
 * @brief Get current song.
 * @param[out] out_path Path to current playing song.
 * @param[in] out_path_length Size of the out_path buffer. Path of the current track needs to fit.
 * @param[out] out \ref MusicCurrentTune
 */
Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out);

/**
 * @brief Clear queue.
 */
Result tuneClearQueue();
Result tuneMoveQueueItem(u32 src, u32 dst);
Result tuneSelect(u32 index);
Result tuneSeek(u32 position);

/**
 * @brief Add track to queue.
 * @note Must not include leading mount name.
 * @note Must match ^(sdmc:/.*.mp3)$
 * @param[in] path Path to file on sdcard.
 */
Result tuneEnqueue(const char *path, TuneEnqueueType type);

Result tuneRemove(u32 index);
Result tuneGetWaveform(s16 *out_buffer, size_t count);
Result tuneApplyTitleFilter();

/** Apply all startup/system-context switches to the running sysmodule. */
Result tuneSetStartupPolicy(const TuneStartupPolicy *policy);

/** Read or atomically apply the live five-band equalizer. */
Result tuneGetEqualizerSettings(TuneEqualizerSettings *settings);
Result tuneSetEqualizerSettings(const TuneEqualizerSettings *settings);

Result tuneQuit();

Result tuneGetApiVersion(u32 *version);

#ifdef __cplusplus
}
#endif
