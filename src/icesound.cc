/*
 * IceWM - IceSound
 *
 * Based on IceWM code Copyright (C) 1997-2001 Marko Macek
 *
 * 2000-02-13: Christian W. Zuckschwerdt  <zany@triq.net>
 * 2000-08-08: Playback throttling. E.g. switch though 
 *             multiple workspaces and get a single sound
 * 2000-10-02: Support for (pre-loaded) server samples (ESounD only)
 *             Removed forking of esd calls
 * 2000-12-11: merged patch by Marius Gedminas
 * 2000-12-16: merged patch by maxim@macomnet.ru
 *             - portable way to rip the children
 * 2001-01-15: merged with IceWM release 1.0.6
 * 2001-01-24: Capt Tara Malina <learfox2@hotmail.com>
 *             Added Y sound support, fixed command line parsing bugs,
 *             fixed conditional and loop braket compiler warnings,
 *             diversified multiple sound system target support, improved
 *             command line argument parsing and handling, fixed
 *             multiple module referancing of global resources (in this
 *             module), diversified ESounD resource referances handling,
 *             added comments, fixed main() return, fixed signal watching
 *             to handle SIGTERM (removed SIGKILL since that is never sent).
 *
 * for now get latest patches as well as sound and cursor themes at
 *  http://triq.net/icewm/
 *
 * Sound output interface types and notes:
 *
 *	OSS	Open Sound Source (generic)
 *	Y	Y sound systems
 *	ESounD	Enlightenment Sound Daemon (uses caching and *throttling)
 *
 * `*' Throttling is max. one sample every 500ms. Comments?
 */
#include "config.h"

#include <stdio.h>

/* Short-cut if either GUIEVENTS or ICESOUND are not requested */
#ifndef CONFIG_GUIEVENTS
int
main(int argc, char *argv[]) {
 printf("%s not supported. Configure with '--enable-guievents'.\n", argv[0]);
}
#else /* CONFIG_GUIEVENTS */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <signal.h>

#define	GUI_EVENT_NAMES
#include "guievent.h"
#define GUIEVENTS	(int)(sizeof(gui_events) / sizeof(gui_events[0]))

#include "base.h" /* strJoin */
#include "yapp.h"

#ifndef DIR_DELIMINATOR
# define DIR_DELIMINATOR	'/'
#endif

#ifndef ISBLANK
# define ISBLANK(c)	(((c) == ' ') || ((c) == '\t'))
#endif

/* Be verbose, if set to 1 then print out each sound play event
 * to stdout.
 */
static int be_verbose = 0;

/* Specifies the sound output interface type, valid values are as
 * follows:
 *
 *	-1 = None/error.
 *	 0 = Generic (OSS).
 *	 1 = Y2 (if ENABLE_Y is defined).
 *	 2 = ESounD (if ESD is defined).
 */
static int sound_output_type = -1;


/* Program file name (without full path). */
char const * YApplication::Name = NULL;
static char const *& program_name(YApplication::Name);

/* Sounds directory which contains the set of sounds this program
 * will have the target sound system play.
 * Fallback to $HOME/.icewm/sounds then LIBDIR/sounds.
 */
static const char *sounds_dir = NULL;

/* OSS digital signal processor, used only if interface is set
 * to OSS. If DSP_device is NULL then it implies it was never selected
 * to be used.
 */
#define DEF_DSP_DEVICE	"/dev/dsp"
static const char *DSP_device = NULL;

/* General and OSS functions. */
static void dsp_sound_play(const char *device, int sound);
/* static void fork_sound_play(int sound); */	/* Not used. */
static void clean_exit(int sig);
static void sig_hup(int sig);
static void sig_chld(int sig);
static int argcmp(const char *string, const char *arg);
static void print_help();


/* *************************** Y Section ************************** */
/* If ENABLE_Y is defined then the Y2 header files will be included
 * and the functions for Y server interfacing will be declared here
 * in this section.
 */
#ifdef ENABLE_Y

#include <Y2/Y.h>
#include <Y2/Ylib.h>

/* Note all Y variables global to this module start with `Y_'.
 * Also note that if variable Y_server is not NULL then it implies we are
 * using the Y server.
 */
static YConnection *Y_server = NULL;		/* Connection to Y server. */
static const char *Y_recorder = NULL;		/* Address:port string. */
static const char *Y_audio_mode = NULL;		/* Audio mode name. */
static Boolean Y_audio_mode_auto = False;	/* Automantically adjust Audio. */

static void Y_print_audio_modes(const char *recorder);
static void Y_play(
        YConnection *con, const char *path,
        Coefficient c_left, Coefficient c_right,
        Boolean match_audio_mode
);

/*
 *	Prints list of preset Audio modes on the Y server specified at
 *	recorder (which may be NULL).
 */
static void Y_print_audio_modes(const char *recorder)
{
	YConnection *con;

	/* Check if given Y server address is NULL, if it is then
	 * get address from environment RECORDER. If that is not available
	 * then assume "127.0.0.1:9433".
	 */
	if(recorder == NULL)
	{
	    recorder = (const char *)getenv("RECORDER");
	    if(recorder == NULL)
		recorder = "127.0.0.1:9433";
	}
	/* Connect to Y server. */
	con = YOpenConnection(
	    NULL,		/* No auto start. */
	    recorder		/* Connect address. */
	);
	if(con != NULL)
	{
	    YAudioModeValuesStruct **m, *m_ptr;
	    int i, total;

	    m = YGetAudioModes(con, &total);
	    if(m != NULL)
	    {
		for(i = 0; i < total; i++)
		{
		    m_ptr = m[i];
		    if(m_ptr == NULL)
			continue;
/* Need to make print out more neat. */
		    printf(
			"%s: %i Hz, %i bits, %i ch\n",
			m_ptr->name,
			m_ptr->sample_rate,
			m_ptr->sample_size,
			m_ptr->channels
		    );
		}

		YFreeAudioModesList(m, total);
	    }
	    YCloseConnection(con, False);
	}
	else
	{
            /* Connect failed, print reason to stderr. */
            fprintf(
                stderr,
                "Cannot connect to Y server: "
	    );
            if(recorder == NULL)
            {
                if(getenv("RECORDER") == NULL)
                    fprintf(
                        stderr,
                        "$RECORDER variable not set."
                    );
                else
                    fprintf(
                        stderr,
                        "connect failed."
                    );
            }
            else
            {
                fprintf(
                    stderr,
                    "%s: connect failed.",
                    recorder
                );
            }
            fputc('\n', stderr);
	}

	return;
}

/*
 *	Plays the specified path to the Y server connected on con.
 */
static void Y_play(
	YConnection *con, const char *path,
	Coefficient c_left, Coefficient c_right,
	Boolean match_audio_mode
)
{
	char *play_path = NULL;
	YEventSoundPlay value;
	long cycle;


	if((con == NULL) || (path == NULL))
	    return;

	if(c_left < 0.0)
	    c_left = 0.0;
	if(c_right < 0.0)
	    c_right = 0.0;

	/* Given path not absolute? */
	if((*path) != DIR_DELIMINATOR)
	{
	    /* Not absolute path, check if sounds_dir was given. */
	    if(sounds_dir == NULL)
	    {
		/* Check in home dir. */
		const char *home = (const char *)getenv("HOME");
		if(home != NULL)
		{
		    char *strptr = strJoin(home, "/.icewm/sounds/", NULL);
		    if(strptr != NULL)
		    {
			play_path = strJoin(strptr, path, NULL);
			delete[] strptr;
		    }
		}
#ifdef DEBUG
		if(1)
#else
		if((play_path != NULL) && be_verbose)
#endif	/* DEBUG */
		    printf("%s\n", play_path);

		/* Exists in home dir? */
		if(play_path ? (access(play_path, R_OK) != 0) : 1)
		{
		    /* Does not exist in home dir, check global dir. */
		    char *libpath;

		    if(play_path != NULL)
			delete[] play_path;

		    /* Format path. */
		    libpath = strJoin(LIBDIR, "/sounds/", NULL);
		    if(libpath != NULL)
		    {
			play_path = strJoin(libpath, path, NULL);
			delete[] libpath;
		    }
		    else
		    {
			play_path = NULL;
		    }
#ifdef DEBUG
                    if(1)
#else
                    if((play_path != NULL) && be_verbose)
#endif  /* DEBUG */
                        printf("%s\n", play_path);

		    /* Exists in global dir? */
		    if(play_path ? (access(play_path, R_OK) != 0) : 1)
		    {
			if(play_path != NULL)
			    delete[] play_path;
			play_path = NULL;
		    }
		}
	    }
	    else
	    {
		/* Check in sounds_dir. */
		play_path = strJoin(sounds_dir, path, NULL);
#ifdef DEBUG    
                if(1)
#else   
                if((play_path != NULL) && be_verbose)
#endif  /* DEBUG */
                    printf("%s\n", play_path);

                /* Exists? */
		if(play_path ? (access(play_path, R_OK) != 0) : 1)
		{
		    if(play_path != NULL)
			delete[] play_path;
                    play_path = NULL;
		}
	    }
	}

	/* Play path not available? */
	if(play_path == NULL)
	    return;

	if(match_audio_mode)
	{
	    /* Get listing of Audio modes available. */
	    int mc, mt;
            YAudioModeValuesStruct **m, *mp, *matched_mode = NULL;
	    YEventSoundObjectAttributes au_stats;


            /* Get sound object attributes. */
	    if(YGetSoundObjectAttributes(con, play_path, &au_stats))
	    {
		delete[] play_path;
		return;
	    }

            m = YGetAudioModes(con, &mt);
            for(mc = 0; mc < mt; mc++)
            {
                mp = m[mc];
                if(mp == NULL)
                    continue;

                if((mp->sample_rate == au_stats.sample_rate) &&
                   (mp->channels == au_stats.channels) &&
                   (mp->sample_size == au_stats.sample_size)
                )
                {
                    matched_mode = mp;
                    break;
                }
	    }
            if(matched_mode == NULL)
            {
                /* No Audio mode matched, try to set explicit Audio
		 * values.
		 */
                cycle = YCalculateCycle(
		    con,
		    au_stats.sample_rate,	/* Sample rate. */
		    au_stats.channels,		/* Channels. */
		    au_stats.sample_size,	/* Bits. */
		    4096			/* Buffer fragment size. */
		);
		if(cycle < 100)
		    cycle = 100;

		YSetAudioModeValues(
		    con,
		    au_stats.sample_size,	/* Bits. */
		    au_stats.channels,		/* Channels. */
		    au_stats.sample_rate,	/* Sample rate. */
		    0,				/* Play. */
		    1,				/* Allow multiple buffers. */
		    2,				/* Number of buffers. */
		    4096			/* Buffer size. */
		);
		YSyncAll(con, True);
		YSetCycle(con, cycle);
	    }
	    else
	    {
		/* Matched mode. */
		YChangeAudioModePreset(con, matched_mode->name);
	    }

	    YFreeAudioModesList(m, mt);
	}

	/* Set up play values. */
	value.flags = YPlayValuesFlagPosition |
	              YPlayValuesFlagTotalRepeats |
	              YPlayValuesFlagVolume;
	value.position = 0;			/* `From the top' */
	value.total_repeats = 1;		/* Repeat once. */
	value.left_volume = c_left;
	value.right_volume = c_right;
	YStartPlaySoundObject(con, play_path, &value);

	/* Deallocate play path. */
	delete[] play_path;

	return;
}

#endif	/* ENABLE_Y */


/* ************************* ESounD Section *********************** */
/* If ESD is defined then the esd.h header file will be included
 * and the functions for ESounD server interfacing will be declared here
 * in this section.
 */
#ifdef ESD
/* #include <audiofile.h> */		/* just let ESD do it */
#include <esd.h>

/* Note, these are not declared static, implying we want other modules
 * to mess with them.
 * Also note that if variable esd is non-negative then it implies we
 * are using the ESounD server.
 */
char *esd_server = NULL;	/* We should read it as an option. */
int esd = -1;			/* ESounD socket. */
int esd_sample[GUIEVENTS];	/* Cache sample id index. */

/* Functions for ESounD IO. */
static int esd_cache_sound(int fd, char *path, int sound);
static int esd_uncache(int fd);
static int esd_cache(int fd);
static void esd_sound_play(int sound);


/*
 *	Cache a sample in the ESounD server.
 *
 *	Returns sample ID or negative on error.
 */
static int esd_cache_sound(int fd, char *path, int sound)
{
	int ok;

	if(fd < 0)
	    return(-1);

	ok = esd_file_cache(fd, program_name, path);
	if(ok < 0)
	{
	    printf(
		"%s: Error <%d> uploading. Name = %s:%s\n",
		program_name, ok, program_name, path
	    );
	    return(ok);
	}
	else
	{
	    esd_sample[sound] = ok;
	    printf(
		"%s: Sample <%d> uploaded. Name = %s:%s\n",
		program_name, ok, program_name, path
	    );
	    return(ok);
	}
}

/*
 *	Uncache all samples from the ESounD server.
 *
 *	Returns number of samples catched.
 */
static int esd_uncache(int fd)
{
	int i, cnt;

	if(fd < 0)
	    return(0);

	for(i = 0, cnt = 0; i < (int)GUIEVENTS; i++)
	{
	    if(esd_sample[i] > 0)
	    {
		esd_sample_free(fd, esd_sample[i]);
		cnt++;
	    }
	}

	return(cnt);
}

/*
 *	Cache the sounds into the EsounD server.
 *
 *	Returns the number of loaded samples.
 */
static int esd_cache(int fd)
{
	int cnt = 0;
	const char *homeDir = getenv("HOME");
	char *path, *libpath;

        char sound_file[1024];
        char *sound_path;


	if(fd < 0)
	    return(0);

	path = strJoin(homeDir, "/.icewm/sounds/", NULL);
	libpath = strJoin(LIBDIR, "/sounds/", NULL);

	/* Itterate through GUI events. */
	for(int i = 0; i < (int)GUIEVENTS; i++)
	{
	    sprintf(sound_file, "%s.wav", gui_events[i].name);
	    if(sounds_dir != NULL)
	    {
		sound_path = strJoin(sounds_dir, sound_file, NULL);
		if(access(sound_path, R_OK) == 0)
		{
		    esd_cache_sound(fd, sound_path, i);

		    delete[] sound_path;
		    sound_path = NULL;

		    cnt++;
		    continue;
		}
		else
		{
		    delete[] sound_path;
                    sound_path = NULL;
		}
	    }
	    sound_path = strJoin(path, sound_file, NULL);
	    if(access(sound_path, R_OK) == 0)
	    {
		esd_cache_sound(fd, sound_path, i);

                delete[] sound_path;
                sound_path = NULL;

		cnt++;
	    }
	    else
	    {
		delete[] sound_path;
		sound_path = strJoin(libpath, sound_file, NULL);
		if(access(sound_path, R_OK) == 0)
		{
		    esd_cache_sound(fd, sound_path, i);

                    delete[] sound_path;
                    sound_path = NULL;

		    cnt++;
		}
		else
		{
                    delete[] sound_path;
                    sound_path = NULL;
		}
	    }
	}	/* Itterate through GUI events. */

	/* Deallocate strings. */
	if(path != NULL)
	    delete[] path;
	if(libpath != NULL)
	    delete[] libpath;
	if(sound_path != NULL)
	    delete[] sound_path;

	return(cnt);
}

/*
 *	Play a cached sound sample using ESounD.
 */
void esd_sound_play(int sound)
{
#ifdef DEBUG
	if(1)
#else
	if(be_verbose && (sound > -1))
#endif	/* DEBUG */
	{
	    printf("esd sample play: #%d\n", sound);
	}

	if(esd_sample[sound] > 0)
	    esd_sample_play(esd, esd_sample[sound]);

	return;
}
#endif /* ESD */

/* *************************** General Section ********************** */

/*
 *	Play a sound sample directly to the digital signal processor
 *	specified by device.
 */
static void dsp_sound_play(const char *device, int sound)
{
	int i;

	if((device == NULL) || (sound < 0))
	    return;

	for(i = 0; i < GUIEVENTS; i++)
	{
	    if(gui_events[i].type == sound)
	    {
		char s[1024 + 256];
		if(sounds_dir != NULL)
		{
		    sprintf(
			s, "%s/%s.wav",
			sounds_dir, gui_events[i].name
		    );
		}
		else
		{
		    /* !!! TODO: search in ($HOME/.icewm/) */

		    sprintf(
			s, "%s/sounds/%s.wav",
			LIBDIR, gui_events[i].name
		    );
		}

#ifdef DEBUG
		if(1)
#else
		if(be_verbose)
#endif	/* DEBUG */
		    printf("sample play: %s\n", s);

		/* Check if sound file to be played exists. */
		if(access(s, R_OK) == 0)
		{
		    int ifd = open(s, O_RDONLY);
		    if(ifd == -1)
			return;

		    int ofd = open(device, O_WRONLY);
		    if(ofd == -1)
		    {
			close(ifd);
			return;
		    }
#ifdef DEBUG
		    printf("copying sound %s to %s\n", s, device);
#endif
		    int n;
		    while((n = read(ifd, s, sizeof(s))) > 0)
			write(ofd, s, n);

		    close(ofd);
		    close(ifd);
		}
		break;
	    }
	}
	return;
}

/*
 *	Fork and play sound sample.
 */
/* Currently not used.
static void fork_sound_play(int sound)
{
	int pid = -1;

	if(sound != -1 && (pid = fork()) == 0)
	{
#ifdef DEBUG
	   printf ("fork for sound: #%d\n", sound);
#endif
	   exit(0);
	}

	return;
}
*/

/* Is sharing of these variables with other modules really needed? If not
 * these should be declared static.
 */
char *displayName		= NULL;
Display *display		= NULL;
Window root			= 0;
struct timeval last;
Atom _GUI_EVENT;

/*
 *	Signal handler.
 */
volatile int terminate = 0;
static void clean_exit(int sig)
{
	signal(sig, SIG_DFL);
	fprintf(
	    stderr,
	    "%s: received signal %d: terminating...\n",
	    program_name, sig
	);
	terminate = 1;
	return;
}

/*
 *	SIGHUP signal handler, restarts and reloads data.
 */
static void sig_hup(int sig)
{
	switch(sig)
	{
	  case SIGHUP:
#ifdef DEBUG
	    printf("%s: signal %i\n", program_name, sig);
#endif	/* DEBUG */
	    /* Reload sound output server's resources. */
#ifdef ENABLE_Y
	    if(Y_server != NULL)
	    {
		/* Nothing to reload. */
	    }
#endif	/* ENABLE_Y */
#ifdef ESD
	    if(esd > -1)
	    {
		esd_uncache(esd);
		esd_cache(esd);
	    }
#endif	/* ESD */
	    break;

	  default:
            fprintf(
                stderr,
 "%s: sig_chld(): Internal error, recieved signal %i is not SIGHUP.\n",
                program_name,
                sig
            );
	    break;
	}

	return;
}

/*
 *	SIGCHLD signal handler.
 */
static void sig_chld(int sig)
{
	pid_t pid;
	int stat;

	if(sig != SIGCHLD)
	{
	    fprintf(
		stderr,
 "%s: sig_chld(): Internal error, recieved signal %i is not SIGCHLD.\n",
		program_name,
		sig
	    );
	    return;
	}

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
#ifdef DEBUG
	    printf("child %d terminated\n", pid);
#endif
	}

	return;
}


/*
 *	Returns non-zero if arg is a prefix of string.
 *
 *	Ie the following will match and return 1 with respective given
 *	arguments:
 *
 *		"--interface=/tmp" "--interface"
 */
static int argcmp(const char *string, const char *arg)
{
        if((string == NULL) ||
           (arg == NULL)
        )
            return(0);
      
        if((*arg) == '\0')
            return(0);

        while((*arg) != '\0')
        {
            if(toupper(*string) != toupper(*arg))
                return(0);

            string++;
            arg++;
        }

        return(1);
}

/*
 *	Prints help screen.
 */
static void print_help()
{
	printf(
	    "\
Usage: %s [options]\n\
\n\
    Where [options] can be any of the following:\n\
\n\
        --sample-dir <dir>      Specifies the directory which contains\n\
                                the sound files (ie ~/.icewm/sounds).\n\
        -s                      Same as --sample-dir.\n\
        --interface <target>    Specifies the sound output target\n\
                                interface, one of; OSS, Y, ESounD\n\
        -i                      Same as --interface.\n\
        --device <device>       (OSS only) specifies the digital signal\n\
                                processor (default /dev/dsp).\n\
        -d                      Same as --device.\n\
        --recorder <addr:port>  (Y only) specifies Y server address and\n\
                                port number (default localhost:9433).\n\
                                This parameter must be specified before\n\
                                any other `Y only' parameters.\n\
        --audio-mode <mode>     (Y only) specifies the Audio mode (leave\n\
                                blank to get a list).\n\
        --m                     Same as --audio-mode.\n\
	--audio-mode-auto       (Y only) change Audio mode on the fly to\n\
                                best match sample's Audio (can cause\n\
                                problems with other Y clients, overrides\n\
                                --audio-mode).\n\
        --verbose               Be verbose (prints out each sound event to\n\
                                stdout).\n\
        -v                      Same as --verbose.\n\
        --help                  Prints (this) help screen and exits.\n\
        -h                      Same as --help.\n\
        --version               Prints version information and exits.\n\
\n\
    Return values:\n\
\n\
        0       Success.\n\
        1       General error.\n\
        2       Command line error.\n\
        3       Subsystems error (ie cannot connect to server).\n\
\n",
	    program_name
	);

	return;
}


/*
 *	Main.
 */
int main(int argc, char *argv[])
{
#ifdef DEBUG

#endif	/* DEBUG */

        /* Get program name (without full path). */
        if((argc > 0) ? (argv[0] != NULL) : 0)
        {
            const char *pn = strrchr(argv[0], DIR_DELIMINATOR);
            program_name = (const char *)((pn) ? (pn + 1) : (argv[0]));
        }

#ifdef DEBUG
	printf(
"%s has been compiled with DEBUG, debugging messages will be printed.\n",
	    program_name
	);
#endif  /* DEBUG */


	/* Parse arguments. */
	for(int i = 1; i < argc; i++)
	{
	    const char *arg_ptr = (const char *)argv[i];
	    if(arg_ptr == NULL)
		continue;

	    /* Print help? */
	    if(argcmp(arg_ptr, "--help") ||
               argcmp(arg_ptr, "-help") ||
               argcmp(arg_ptr, "--h") ||
               argcmp(arg_ptr, "-h") ||
               argcmp(arg_ptr, "--?") ||
               argcmp(arg_ptr, "-?")
            )
	    {
		print_help();
		return(0);
	    }
            /* Print version? */
            else if(argcmp(arg_ptr, "--version") ||
                    argcmp(arg_ptr, "-version")
            )
            {
		printf(
#ifdef VERSION
		    "%s\n", VERSION
#else
		    "Version not defined at compile time.\n"
#endif
		);
                return(0);
            }
	    /* Sounds (`samples') directory. */
	    else if(argcmp(arg_ptr, "--sample-dir") ||
                    argcmp(arg_ptr, "-s")
            )
	    {
		const char *val_ptr = (const char *)strchr(arg_ptr, '=');
		if(val_ptr == NULL)
		{
		    /* Value assumed on next argument. */
		    i++;
		    if(i < argc)
		    {
			arg_ptr = (const char *)argv[i];
			sounds_dir = arg_ptr;
		    }
		    else
		    {
			fprintf(
			    stderr,
			    "%s: Requires argument.\n",
			    arg_ptr
			);
			return(2);
		    }
		}
		else
		{
		    val_ptr++;			/* Seek past '='. */
		    while(ISBLANK(*val_ptr))	/* Seek past any spaces. */
			val_ptr++;
		    sounds_dir = val_ptr;	/* Get value. */
		}
	    }
	    /* Sound output interface type. */
	    else if(argcmp(arg_ptr, "--interface") ||
                    argcmp(arg_ptr, "-i")
	    )
	    {
		const char *sot = NULL;
                const char *val_ptr = (const char *)strchr(arg_ptr, '=');
                if(val_ptr == NULL)
                {   
                    /* Value assumed on next argument. */
                    i++;
                    if(i < argc)
                    {
                        arg_ptr = (const char *)argv[i];
                        sot = arg_ptr;
                    }
                    else
                    {
                        fprintf(
                            stderr,
                            "%s: Requires argument.\n",
                            arg_ptr
                        );
                        return(2);
                    }
                }
                else
                {
                    val_ptr++;                  /* Seek past '='. */
                    while(ISBLANK(*val_ptr))    /* Seek past any spaces. */
                        val_ptr++;
                    sot = val_ptr;       /* Get value. */
                }

		/* Got sound output type name string? */
		if(sot != NULL)
		{
		    if(sound_output_type > -1)
		    {
			fprintf(
			    stderr,
 "Warning: Multiple settings of sound output types given.\n"
			);
		    }

		    /* Generic (OSS)? */
		    if(!strcmp(sot, "OSS"))
		    {
			sound_output_type = 0;
		    }
		    /* Y2? */
                    else if(!strcmp(sot, "Y") ||
                            !strcmp(sot, "Y2") ||
                            !strcmp(sot, "YIFF")
		    )
                    {
#ifdef ENABLE_Y
                        sound_output_type = 1;
#else
			fprintf(
			    stderr,
 "%s: Support for this interface was not compiled.\n",
			    sot
			);
			return(2);
#endif
                    }
                    /* ESounD? */
                    else if(!strcmp(sot, "ESD") ||
                            !strcmp(sot, "ESounD")
                    )
                    {
#ifdef ESD
                        sound_output_type = 2;
#else
                        fprintf(
                            stderr,
 "%s: Support for this interface was not compiled.\n",
                            sot
                        );
			return(2);
#endif
                    }
		    /* Unsupported interface. */
		    else
		    {
                        fprintf(
                            stderr,
			    "%s: Unsupported interface.\n",
                            sot
                        );
			return(2);
		    }
		}
	    }
	    /* Generic (OSS) digital signal processor. */
            else if(argcmp(arg_ptr, "--device") ||
                    argcmp(arg_ptr, "-device") ||
                    argcmp(arg_ptr, "-d")
            )
            {
                const char *val_ptr = (const char *)strchr(arg_ptr, '=');
                if(val_ptr == NULL)
                {
                    /* Value assumed on next argument. */
                    i++;
                    if(i < argc)
                    {
                        arg_ptr = (const char *)argv[i];
                        DSP_device = arg_ptr;
                    }
                    else
                    {
                        fprintf(
                            stderr,
                            "%s: Requires argument.\n",
                            arg_ptr
                        );
                        return(2);
                    }
                }
                else
                {
                    val_ptr++;                  /* Seek past '='. */
                    while(ISBLANK(*val_ptr))    /* Seek past any spaces. */
                        val_ptr++;
                    DSP_device = val_ptr;       /* Get value. */
                }
            }


#ifdef ENABLE_Y
            /* Y recorder address and port. */
            else if(argcmp(arg_ptr, "--recorder") ||
                    argcmp(arg_ptr, "-recorder")
            )
            {
                const char *val_ptr = (const char *)strchr(arg_ptr, '=');
                if(val_ptr == NULL)
                {
                    /* Value assumed on next argument. */
                    i++;
                    if(i < argc)
                    {
                        arg_ptr = (const char *)argv[i];
                        Y_recorder = arg_ptr;
                    }
                    else
                    {
                        fprintf(
                            stderr,
                            "%s: Requires argument.\n",
                            arg_ptr
                        );
                        return(2);
                    }
                }
                else
                {
                    val_ptr++;                  /* Seek past '='. */
                    while(ISBLANK(*val_ptr))    /* Seek past any spaces. */
                        val_ptr++;
                    Y_recorder = val_ptr;       /* Get value. */
                }
	    }
            /* Automatically change Y Audio mode to best fit sample
	     * about to be played.
	     */
            else if(argcmp(arg_ptr, "--audio-mode-auto"))
            {
                Y_audio_mode_auto = True;
                if(Y_audio_mode != NULL)
                {
                    fprintf(
                        stderr,
"Warning: %s overrides previous set Audio mode `%s'.\n",
                        arg_ptr,
                        Y_audio_mode
                    );
                }
            }
            /* Y Audio mode. */
            else if(argcmp(arg_ptr, "--audio-mode") ||
                    argcmp(arg_ptr, "-m")
            )
	    {
                const char *val_ptr = (const char *)strchr(arg_ptr, '=');
                if(val_ptr == NULL)
                {
                    /* Value assumed on next argument. */
                    i++;
                    if(i < argc)
                    {
                        arg_ptr = (const char *)argv[i];
                        Y_audio_mode = arg_ptr;

			/* If giveniven audio mode is "?", then print
			 * Audio modes list.
			 */
			if(!strcmp(arg_ptr, "?"))
			{
			    Y_print_audio_modes(Y_recorder);
			    return(0);
			}
                    }
                    else
                    {
			/* Print list of Y audio modes and exit. */
			Y_print_audio_modes(Y_recorder);
                        return(0);
                    }
                }
                else
                {    
                    val_ptr++;                  /* Seek past '='. */
                    while(ISBLANK(*val_ptr))    /* Seek past any spaces. */
                        val_ptr++;
                    Y_audio_mode = val_ptr;	/* Get value. */
                }
	    }

#endif	/* ENABLE_Y */

            /* Verbose. */
            else if(argcmp(arg_ptr, "--verbose") ||
                    argcmp(arg_ptr, "-v")
	    )
            {
		be_verbose = 1;
            }
	    /* Unrecognized value. */
	    else
	    {
		fprintf(
		    stderr,
		    "%s: Unrecognized parameter.\n",
		    arg_ptr
		);
		return(2);
	    }
	}

	/* Set up signals, terminate if any of these signals are
	 * recieved.
	 */
	signal(SIGINT, clean_exit);
	signal(SIGTERM, clean_exit);
	signal(SIGPIPE, clean_exit);


	/* Reset each sound output interface target type's resources,
	 * it's safe to do this before they are initialized and even if
	 * they are not selected as the interface target type to use.
	 */
#ifdef ENABLE_Y
	Y_server = NULL;	/* Connection to Y server. */
#endif	/* ENABLE_Y */
#ifdef ESD
	/* Reset ESounD sample array to -1. */
	for(int i = 0; i < (int)GUIEVENTS; i++)
	    esd_sample[i] = -1;
	esd = -1;		/* Connection to ESounD server. */
#endif	/* ESD */


	/* If the sound output type was not set in the command line
	 * arguments then assume 0 for OSS.
	 */
	if(sound_output_type < 0)
	    sound_output_type = 0;


	/* Begin initializing sound resources depending on
	 * sound_output_type.
	 */
#ifdef ENABLE_Y
	YConnection *con;
#endif	/* ENABLE_Y */
#ifdef ESD
	int fd;
#endif	/* ESD */
	switch(sound_output_type)
	{
	  case 2:	/* ESounD */
#ifdef ESD
	    /* Attempt to open ESounD server. */
	    fd = esd_open_sound(esd_server);
	    if(fd > -1)
	    {
		/* Connected! */

		/* Cache ESounD samples. */
		esd_cache(fd);

		/* Now set descriptor to server, this is incase a SIGHUP
		 * or other async event occured during initialization.
		 */
		esd = fd;
	    }
	    else
	    {
                /* Connect failed, print reason to stderr. */
                fprintf(
                    stderr,
                    "Cannot connect to ESounD server: "
		);
		if(esd_server == NULL)
		{
                    fprintf(
                        stderr,
                        "connect failed."
                    );
                }
                else
                {
                    fprintf(
                        stderr,
                        "%s: connect failed.",
                        esd_server
		    );
		}
                fputc('\n', stderr);

		return(3);
	    }
#endif	/* ESD */
	    break;

	  case 1:	/* Y */
#ifdef ENABLE_Y
	    /* Check if Y server address is given, if not then use
	     * environment RECORDER value. If that is also not available
	     * then assume "127.0.0.1:9433".
	     */
	    if(Y_recorder == NULL)
	    {
		Y_recorder = (const char *)getenv("RECORDER");
		if(Y_recorder == NULL)
		    Y_recorder = "127.0.0.1:9433";
	    }
	    /* Connect to Y server. */
            con = YOpenConnection(
		NULL,		/* No auto start. */
		Y_recorder	/* Connect address. */
	    );
	    if(con != NULL)
	    {
                /* Connected! */

		/* Command line specified Audio mode? */
		if(Y_audio_mode != NULL)
		{
		    /* Change to specified preset Audio mode. */
		    if(YChangeAudioModePreset(con, Y_audio_mode))
                        fprintf(
                            stderr,
                    "%s: %s: Cannot change to Audio mode.\n",
                            program_name, Y_audio_mode
                        );
		}

                /* Now set descriptor to server, this is incase a SIGHUP
                 * or other async event occured during initialization.
                 */
		Y_server = con;
            }
            else
            {
                /* Connect failed, print reason to stderr. */
                fprintf(
                    stderr,
                    "Cannot connect to Y server: "
                );
                if(Y_recorder == NULL)
                {
                    if(getenv("RECORDER") == NULL)
                        fprintf(
                            stderr,
                            "$RECORDER variable not set."
                        );
                    else
                        fprintf(
                            stderr,
                            "connect failed."
                        );
                }
                else    
                {
                    fprintf(
                        stderr,
                        "%s: connect failed.",
                        Y_recorder
                    );
                }
                fputc('\n', stderr);

		return(3);
	    }
#endif	/* ENABLE_Y */
	    break;

	  case 0:	/* OSS */
	    if(DSP_device == NULL)
		DSP_device = DEF_DSP_DEVICE;
	    if(access(DSP_device, W_OK) != 0)
	    {
                fprintf(
                    stderr, 
                    "%s: No such device.",
                    DSP_device
		);
		DSP_device = NULL;
		return(3);
	    }
	    break;

	  default:
	    fprintf(
		stderr,
		"%s: Internal error: Unsupported sound target type `%i'.\n",
		program_name,
		sound_output_type
	    );
	    break;
	}



	/* Connect to X server. */
	display = XOpenDisplay(displayName);
	if(display == NULL)
	{
	    /* Connect failed, print reason to stderr. */
	    fprintf(
		stderr,
		"Cannot connect to X display: "
	    );
	    if(displayName == NULL)
	    {
		if(getenv("DISPLAY") == NULL)
		    fprintf(
			stderr,
			"$DISPLAY variable not set."
		    );
		else
		    fprintf(
			stderr,
			"connect failed."
		    );
	    }
	    else
	    {
		fprintf(
		    stderr,
		    "%s: connect failed.",
		    displayName
		);
	    }
	    fputc('\n', stderr);

	    return(3);
	}

	root = RootWindow(display, DefaultScreen(display));

	_GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);

	XSelectInput(display, root, PropertyChangeMask);

	signal(SIGCHLD, sig_chld);
	signal(SIGHUP, sig_hup);

	gettimeofday(&last, NULL);

	/* Manage main while loop while local global terminate remains
	 * false and the X server connection is up.
	 */
	while(!terminate && (display != NULL))
	{
	    XEvent xev;
	    struct timeval now;

#ifdef ENABLE_Y
	    /* Manage Y events? */
	    if(Y_server != NULL)
	    {
		YEvent yev;

		/* Get next Y event (non-blocking). */
		while((Y_server != NULL) &&
                      (YGetNextEvent(Y_server, &yev, False) > 0)
		)
		{
		    /* Got a Y event, now handle by its type. */
		    switch(yev.type)
		    {
		      case YAudioChange:
#ifdef DEBUG
			if(yev.audio.preset)
			{
			    printf(
 "%s: Y server switched to preset Audio `%s'\n",
				program_name, yev.audio.mode_name
			    );
			}
			else
			{
                            printf(
 "%s: Y server changed Audio to; %i Hz, %i bits, %i ch, %i bytes\n",
                                program_name,
				yev.audio.sample_rate,
				yev.audio.sample_size,
				yev.audio.channels,
				yev.audio.fragment_size_bytes
                            );
			}
#endif	/* DEBUG */
			/* If we are automatically adjusting the Audio
			 * mode and some other Y client or the Y server
			 * itself has changed the Audio then we should
			 * step automatic changing of Audio mode to
			 * prevent `fighting' with the other Y client or
			 * Y server.
			 */
			if(Y_audio_mode != NULL)
			{
			    fprintf(
				stderr,
"%s: Detected Audio change, initial Audio mode `%s' no longer in affect.\n",
				program_name, Y_audio_mode
			    );
			    Y_audio_mode = NULL;
			}
			if(Y_audio_mode_auto)
			{
                            fprintf(
                                stderr,
"%s: Detected Audio change, automatic Audio mode changing disabled.\n",
                                program_name
                            );
			    Y_audio_mode_auto = False;
			}
			break;

		      case YSoundObjectKill:
#ifdef DEBUG
                        printf(
 "%s: Y sound object #%ld finished playing.\n",
                            program_name, yev.kill.yid
                        ); 
#endif	/* DEBUG */
			break;

		      case YDisconnect:
#ifdef DEBUG
                        printf(
 "%s: Lost connection to Y server, reason `%i'\n",
                            program_name, yev.disconnect.reason
                        );
#endif	/* DEBUG */
                        /* Y server has disconnected us, we need to close
                         * the connection and mark as disconnected.
                         */
                        YCloseConnection(Y_server, False);
                        Y_server = NULL;
                        break;

		      case YShutdown:
#ifdef DEBUG            
                        printf(
 "%s: Y server has shutdown, reason `%i'\n",
                            program_name, yev.shutdown.reason
                        );
#endif	/* DEBUG */
			/* Y server has shutdown, we need to close
			 * the connection and mark as disconnected.
			 */
			YCloseConnection(Y_server, False);
			Y_server = NULL;
			break;
		    }
		}

		/* Has the Y server been detected to disconnected us or
		 * shutdown in the above event handling?
		 */
		if(Y_server == NULL)
		{
		    /* Yes, so now we need to terminate. */
		    terminate = 1;
		    continue;
		}
	    }
#endif	/* ENABLE_Y */


	    /* Check if any X events exist, if not then do not
	     * continue further.
	     */
	    if(XPending(display) <= 0)
	    {
		usleep(10000);
		continue;
	    }

	    /* There is an event, fetch it. */
	    XNextEvent(display, &xev);

	    /* Handle fetched X event by tis type. */
	    switch(xev.type)
	    {
	      case PropertyNotify:
		if(xev.xproperty.atom == _GUI_EVENT)
		{
		    if(xev.xproperty.state == PropertyNewValue)
		    {
			Atom type;
			int format;
			unsigned long nitems, lbytes;
			unsigned char *propdata;
			int d = -1;

			if(XGetWindowProperty(
			    display, root,
			    _GUI_EVENT, 0, 3, False, _GUI_EVENT,
			    &type, &format, &nitems, &lbytes,
			    &propdata) == Success)
			{
			    if(propdata)
			    {
				d = *(char *)propdata;
				XFree(propdata);
			    }
                        }

			/* Recieved restart event? */
			if(geRestart == d)
			{
#ifdef DEBUG
			    printf(
				"%s: recieved restart event.\n",
				program_name
			    );
#endif
			    sig_hup(SIGHUP);
			}

			/* Update timming. */
			gettimeofday(&now, NULL);

			/* Throttle(?) */
			if(((now.tv_sec - last.tv_sec) >= 2) ||
			   ((((now.tv_sec - last.tv_sec) * 1000000) +
			     now.tv_usec - last.tv_usec) > 500000)
			)
			{
		            last.tv_sec = now.tv_sec;
		            last.tv_usec = now.tv_usec;


			    /* Call target sound system output play. */
#ifdef ENABLE_Y
			    /* Use Y? */
                            if(Y_server != NULL)
                            {
				int i;

                                for(i = 0; i < GUIEVENTS; i++)
                                {
                                    if(gui_events[i].type == d)
                                    {
					char *fn;

					if(gui_events[i].name != NULL)
					    fn = strJoin(
						gui_events[i].name, ".wav",
						NULL
					    );
					else
					    fn = NULL;

					Y_play(
					    Y_server, fn,
					    1.0, 1.0,
					    Y_audio_mode_auto
					);

					if(fn != NULL)
					    delete[] fn;
				    }
				}
                            }
#endif	/* ENABLE_Y */
#ifdef ESD
			    /* Use ESounD? */
			    if(esd > -1)
			    {
				esd_sound_play(d);
			    }
#endif	/* ESD */
			    /* Use OSS? */
			    if(DSP_device != NULL)
			    {
				dsp_sound_play(DSP_device, d);
			    /* fork_sound_play(d); */
			    }
			}
		    }
		}
		break;

	    }	/* Handle fetched X event by tis type. */

	}	/* while(!terminate && (display != NULL)) */



	/* Deallocate sound output resources depending on type. */
	switch(sound_output_type)
	{
	  case 2:	/* ESounD */
#ifdef ESD
	    /* Free ESounD resources. */
	    esd_uncache(esd);
	    /* Mark ESounD server connection as closed. */
	    esd = -1;
#endif /* ESD */
	    break;

	  case 1:	/* Y */
#ifdef ENABLE_Y
	    if(Y_server != NULL)
	    {
#ifdef DEBUG
		printf("Disconnecting from Y server...\n");
#endif  /* DEBUG */
		/* Close connection to Y server. */
		YCloseConnection(Y_server, False);
		Y_server = NULL;
	    }
#endif	/* ENABLE_Y */
	    break;

	  case 0:	/* OSS */
	    DSP_device = NULL;
	    break;
	}

	/* Close connection to X server. */
	if(display != NULL)
	{
	    XCloseDisplay(display);
	    display = NULL;
	}

	return(0);
}

#endif /* CONFIG_GUIEVENTS */
