#pragma once
#include <str.h>

struct AudioState
{
	float frequency;
	float volume;
};
static Array<int> current_playing_sound_indexs;
void              init_audio(unsigned int channel,
                             unsigned int sampleRate,
                             unsigned int sound_capacity);

uint32_t init_local_sound(const String &path);

//uint32_t init_local_sound(const char *path);

void play(uint32_t soundIdx);

void stop(uint32_t soundIdx);

void play3D(const float radius,uint32_t soundIdx);

void print_local_sound(char *path);

float get_volume(uint32_t soundIdx);

void set_volume(float volume,uint32_t soundIdx);

void listener_set_position(float x, float y, float z);

void listener_set_direction(float x, float y, float z);

void set_sound_position(float x, float y, float z, uint32_t soundIdx);

void set_sound_velocity(float x, float y, float z, uint32_t soundIdx);

void enable_3Dsound();

void disable_3Dsound();

bool is_3D_enable();

void enable_dynamic_sound();

void disable_dynamic_sound();

bool is_dynamic_sound_enable();

bool is_playing(uint32_t soundIdx);

void stop_all();
void play_all();
void set_volume_for_all(float volume);

void cleanup_audio();
