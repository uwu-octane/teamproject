#include <cstdint>
#include <list.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio.h"
#include <thread>
#include <iostream>
#include <sstream>


AudioState *audioState;

ma_engine engine;
Array<ma_sound> sounds = {};

ma_resource_manager_config config = ma_resource_manager_config_init();

ma_resource_manager resourceManager;

static uint32_t count = 0;



bool                enable_3dsound = 0;
bool enable_dynamic = 0;


//static Array<int> current_playing_sound_indexs;

int listenerIndex;


#define PI 3.141592

void print_local_sound(char *path)
{
	int result;

	ma_format  pFormat;
	ma_uint32  pChannels     = NULL;
	ma_uint32  pSampleRate   = NULL;
	ma_channel pChannelMap   = NULL;
	size_t     channelMapCap = NULL;

	ma_decoder decoder;
	ma_decoder_init_file(path, NULL, &decoder);
	result = ma_decoder_get_data_format(&decoder,
	                                    &pFormat,
	                                    &pChannels,
	                                    &pSampleRate,
	                                    &pChannelMap,
	                                    channelMapCap);

	if (result == 0) {
		std::cout << "channels: " << pChannels
		          << "\nsamplerate: " << pSampleRate << "\n";
	}

	ma_decoder_uninit(&decoder);
}


	void init_audio(unsigned int channel, unsigned int sampleRate, unsigned int sound_capacity)
{
	//audioState              = state;
	ma_engine_config engineConfig;
	ma_resource_manager_init(&config, &resourceManager);
	engineConfig          = ma_engine_config_init();
	engineConfig.noDevice = MA_FALSE;
	engineConfig.channels   = channel;  // Must be set when not using a device.
	engineConfig.sampleRate =
	    sampleRate;  // Must be set when not using a device.
	engineConfig.pResourceManager = &resourceManager;

	ma_result result = ma_engine_init(&engineConfig, &engine);
	if (result != MA_SUCCESS) {
		fprintf(stderr, "ERROR: could not initialize audio engine\n");
		assert(false);
		exit(EXIT_FAILURE);
		return;
	}
	for (int i = 0; i < sound_capacity; i++) { sounds.add({});
	}
	ma_engine_start(&engine);
}


uint32_t init_local_sound(const String &path)
//uint32_t init_local_sound(const char * path)
{
	ma_result   result;
	
	result = ma_sound_init_from_file(&engine,
	                                 path.begin(),
	                                 MA_SOUND_FLAG_DECODE,
	                                 NULL,
	                                 NULL,
	                                 &sounds[count]);
	if (result != MA_SUCCESS) {
		fprintf(stderr, "ERROR: could not initialize sound\n");
		assert(false);
		exit(EXIT_FAILURE);
		//return;
	}
	ma_sound_set_spatialization_enabled(&sounds[count],
	                                    true);
	int temp = count;
	count++;
	
	return temp;
}

void play(uint32_t soundIdx) {
	if (&sounds[soundIdx]) {
		ma_sound_start(&sounds[soundIdx]);
		current_playing_sound_indexs.add(soundIdx);
	} else {
		fprintf(stderr, "ERROR: could play sound\n");
		assert(false);
		exit(EXIT_FAILURE);
	}
 }

void play_all()
 {
	//for (int i = 0; i < sounds.size(); i++) { play(i);
	//}
	for (auto &sound : sounds) { ma_sound_start(&sound);
	}
 }

void stop(uint32_t soundIdx)
 {
	if (&sounds[soundIdx]) {
		ma_sound_stop(&sounds[soundIdx]);
		current_playing_sound_indexs.remove(soundIdx);
	}
 }

void stop_all()
 {
	//for (auto index : current_playing_sound_indexs) { stop(index);
	//}
	for (auto &sound : sounds) {
		if (ma_sound_is_playing) { ma_sound_stop(&sound);
		}
	}
	
 }

void play3D(const float radius, uint32_t soundIdx)
 {
	ma_sound_set_spatialization_enabled(&sounds[soundIdx],
	                                    true);
	if (!ma_sound_is_playing(&sounds[soundIdx])) {
		ma_sound_start(&sounds[soundIdx]);
	}

	float posOnCircle = 0;

	ma_engine_listener_set_position(&engine, 0, 0, 0, 0);
	ma_engine_listener_set_direction(&engine, 0, 0, 0, 1);

	while (enable_3dsound) {
		posOnCircle += 0.04f;
		ma_vec3f position;
		position.x = radius * cosf(posOnCircle);
		position.y = 0;
		position.z = radius * sinf(posOnCircle * 0.5f);

		if (&sounds[soundIdx]) {
			ma_sound_set_position(
			    &sounds[soundIdx],
			                      position.x,
			                      position.y,
			                      position.z);
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ma_sleep(100);
		if (ma_sound_at_end(&sounds[soundIdx])) { break;
		}
		//if (ma_sound_is_playing(&sound)) {
		//	char stringForDisplay[] = "          +         ";
		//	int  charpos = (int)((position.x + radius) / radius * 10.0f);
		//	if (charpos >= 0 && charpos < 20) stringForDisplay[charpos] = 'o';
		//	auto playPos = ma_sound_get_position(&sound);
		//
		//	printf("\rx:(%s)   3dpos: %.1f %.1f %.1f",
		//	       stringForDisplay,
		//	       position.x,
		//	       position.y,
		//	       position.z);
		//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//} else {
		//	break;
		//}
	}
}

float get_volume(uint32_t soundIdx)
{ 
	float current_volume = 0;
	if (&sounds[soundIdx]) {
		current_volume = ma_sound_get_volume(&sounds[soundIdx]);
	}
	return current_volume;
}

void set_volume(float volume, uint32_t soundIdx)
{
	if (&sounds[soundIdx]) { ma_sound_set_volume(&sounds[soundIdx], volume);
	}
 }

void set_volume_for_all(float volume)
 {
	for (auto sound : sounds) {
		if (ma_sound_is_playing) { ma_sound_set_volume(&sound, volume);
		}
	}
 }

void set_sound_position(float x, float y, float z, uint32_t soundIdx)
 {
	ma_sound_set_position(&sounds[soundIdx], x, y, z);
 }

void set_sound_velocity(float x, float y, float z, uint32_t soundIdx)
 {
	ma_sound_set_velocity(&sounds[soundIdx], x, y, z);

}

 void listener_set_position(float x, float y, float z)
 {
	ma_engine_listener_set_position(&engine, 0, x, y, z);
 }

 void listener_set_direction(float x, float y, float z) {
	ma_engine_listener_set_direction(&engine, 0, x, y, z);
 }

void enable_3Dsound() { enable_3dsound = true; }
void disable_3Dsound() { enable_3dsound = false;}

bool is_3D_enable() { return enable_3dsound; }

void enable_dynamic_sound() { enable_dynamic = true; }
void disable_dynamic_sound() { enable_dynamic = false; }

bool is_dynamic_sound_enable() { return enable_dynamic; }

bool is_playing(uint32_t soundIdx)
{
	return ma_sound_is_playing(&sounds[soundIdx]);
}

void cleanup_audio()
{
	for (auto &sound : sounds) { ma_sound_uninit(&sound);
	}

	ma_engine_uninit(&engine);
}