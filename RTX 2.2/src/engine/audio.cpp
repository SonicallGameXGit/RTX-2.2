#include "audio.h"

ALCcontext* TT::AudioSystem::context = NULL;
ALCdevice* TT::AudioSystem::device = NULL;

void TT::AudioSystem::initialize() {
	ALCdevice* device = alcOpenDevice(NULL);
	if (!device) throw std::runtime_error("Could not open OpenAL device.");

	ALCcontext* context = alcCreateContext(device, NULL);
	if (!context) throw std::runtime_error("Could not create OpenAL context.");

	if (!alcMakeContextCurrent(context)) throw std::runtime_error("Could not bind OpenAL context.");
}
void TT::AudioSystem::clear() {
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

ALuint TT::AudioSystem::loadFromFile(const char* location) {
	ALuint buffer;
	
	int channels, sampleRate;
	short* pcm;
	
	int samples = stb_vorbis_decode_filename(location, &channels, &sampleRate, &pcm);
	if (!pcm) throw std::runtime_error(("Could not read file: \"" + std::string(location) + '\"'));
	
	alGenBuffers(1, &buffer);
	alBufferData(buffer, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, samples * channels * sizeof(short), sampleRate);

	free(pcm);

	return buffer;
}
void TT::AudioSystem::clear(ALuint sound) {
	alDeleteBuffers(1, &sound);
}

TT::SoundSource::SoundSource() {
	alGenSources(1, &id);
}

void TT::SoundSource::play(ALuint sound, float volume, float pitch, bool loop) const {
	stop();

	alSourcei(id, AL_BUFFER, sound);
	alSourcef(id, AL_GAIN, volume);
	alSourcef(id, AL_PITCH, pitch);
	alSourcei(id, AL_LOOPING, loop);

	unpause();
}
void TT::SoundSource::clear() {
	alDeleteSources(1, &id);
}

void TT::SoundSource::pause() const {
	alSourcePause(id);
}
void TT::SoundSource::unpause() const {
	alSourcePlay(id);
}
void TT::SoundSource::stop() const {
	alSourceStop(id);
}

bool TT::SoundSource::isPlaying() const {
	ALint state;
	alGetSourcei(id, AL_SOURCE_STATE, &state);

	return state == AL_PLAYING;
}