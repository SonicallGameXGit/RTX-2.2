#pragma once
#include <alc.h>
#include <al.h>
#include <iostream>

#define STB_VORBIS_HEADER_ONLY
#include "../stb/stb_vorbis.c"

namespace TT {
	class AudioSystem {
	public:
		static void initialize();
		static void clear();

		static ALuint loadFromFile(const char* location);
		static void clear(ALuint sound);
	private:
		static ALCcontext* context;
		static ALCdevice* device;
	};
	class SoundSource {
	public:
		SoundSource();
		SoundSource(ALuint sound);

		void setSound(ALuint sound) const;

		void play(float volume, float pitch, bool loop) const;
		void clear();

		void pause() const;
		void unpause() const;
		void stop() const;

		bool isPlaying() const;
	private:
		ALuint id;
	};
}