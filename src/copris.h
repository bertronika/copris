#ifndef REL
	#define COPRIS_RELEASE ""
#else
	#define COPRIS_RELEASE ("-" REL)
#endif

#ifndef FNAME_LEN
	#ifndef NAME_MAX
		#define FNAME_LEN 64
	#else
		#define FNAME_LEN NAME_MAX
	#endif
#endif

void copris_help(char *copris_location);
void copris_version();
