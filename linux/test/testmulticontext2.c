#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define DATABUFFERSIZE (10 * (512 * 3) * 1024)

static void iterate(void);
static void init(const char *fname);
static void cleanup(void);

static ALuint moving_source = 0;

static time_t start;
static void *data = (void *) 0xDEADBEEF;

static ALCcontext *context_id;

static void iterate( void ) {
	static ALfloat position[] = { 10.0f, 0.0f, 4.0f };
	static ALfloat movefactor = 4.5;
	static time_t then = 0;
	time_t now;

	now = time( NULL );

	/* Switch between left and right stereo sample every two seconds. */
	if( now - then > 2 ) {
		then = now;

		movefactor *= -1.0;
	}

	position[0] += movefactor;
	alSourcefv( moving_source, AL_POSITION, position );

	micro_sleep(500000);

	return;
}

static void init( const char *fname ) {
	FILE *fh;
	ALfloat zeroes[] = { 0.0f, 0.0f,  0.0f };
	ALfloat back[]   = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[]  = { 0.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint stereo;
	ALsizei size;
	ALsizei bits;
	ALsizei freq;
	ALsizei format;
	int filelen;
	ALint err;

	data = malloc(DATABUFFERSIZE);

	start = time(NULL);

	alListenerfv(AL_POSITION, zeroes );
	/* alListenerfv(AL_VELOCITY, zeroes ); */
	alListenerfv(AL_ORIENTATION, front );

	alGenBuffers( 1, &stereo);

	fh = fopen(fname, "rb");
	if(fh == NULL) {
		fprintf(stderr, "Couldn't open fname\n");
		exit(1);
	}

	filelen = fread(data, 1, DATABUFFERSIZE, fh);
	fclose(fh);

	alGetError();
	alBufferData( stereo, AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	if( alGetError() != AL_NO_ERROR ) {
		fprintf(stderr, "Could not BufferData\n");
		exit(1);
	}

	free( data );

	alGenSources( 1, &moving_source);

	alSourcefv( moving_source, AL_POSITION, position );
	/* alSourcefv( moving_source, AL_VELOCITY, zeroes ); */
	alSourcei(  moving_source, AL_BUFFER, stereo );
	alSourcei(  moving_source, AL_LOOPING, AL_TRUE);

	return;
}

static void cleanup(void) {
	alcDestroyContext(context_id);
#ifdef JLIB
	jv_check_mem();
#endif
}

int main( int argc, char* argv[] ) {
	ALCdevice *dev;
	int attrlist[] = { ALC_FREQUENCY, 44100,
			   ALC_INVALID };
	time_t shouldend;
	void *dummy;

	dev = alcOpenDevice( NULL );
	if( dev == NULL ) {
		return 1;
	}

	/* Initialize ALUT. */
	context_id = alcCreateContext( dev, attrlist);
	if(context_id == NULL) {
		return 1;
	}

	dummy = alcCreateContext( dev, attrlist);

	alcMakeContextCurrent(dummy);

	alcDestroyContext(context_id);

	fixup_function_pointers();

	talBombOnError();

	if(argc == 1) {
		init("sample.wav");
	} else {
		init(argv[1]);
	}

	alSourcePlay( moving_source );
	while(SourceIsPlaying( moving_source ) == AL_TRUE) {
		iterate();

		shouldend = time(NULL);
		if((shouldend - start) > 10) {
			alSourceStop(moving_source);
		}
	}

	cleanup();

	return 0;
}
