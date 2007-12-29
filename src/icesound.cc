/*
 * IceWM - IceSound
 *
 * Based on IceWM code
 * Copyright (C) 1997-2001 Marko Macek
 * Copyright (C) 2001 The Authors of IceWM
 *
 * 2000-02-13:  Christian W. Zuckschwerdt  <zany@triq.net>
 * 2000-08-08:  Playback throttling. E.g. switch though
 *              multiple workspaces and get a single sound
 * 2000-10-02:  Support for (pre-loaded) server samples (ESounD only)
 *              Removed forking of esd calls
 * 2000-12-11:  merged patch by Marius Gedminas
 * 2000-12-16:  merged patch by maxim@macomnet.ru
 *              portable way to rip the children
 * 2001-01-15:  merged with IceWM release 1.0.6
 * 2001-01-24:  Capt Tara Malina <learfox2@hotmail.com>
 *              Added Y sound support, fixed command line parsing bugs,
 *              fixed conditional and loop braket compiler warnings,
 *              diversified multiple sound system target support, improved
 *              command line argument parsing and handling, fixed
 *              multiple module referancing of global resources (in this
 *              module), diversified ESounD resource referances handling,
 *              added comments, fixed main() return, fixed signal watching
 *              to handle SIGTERM (removed SIGKILL since that is never sent).
 * 2001-03-12:  Mathias Hasselmann <mathias.hasselmann@gmx.de>
 *              GNUified usage message, prepared for NLS, minor cleanups,
 *              made arguments case sensitive, converted into a C++
 *              application
 *
 * TODO: a virtual YAudioInterface class the OSS, ESD and YIFF implement
 *
 * for now get latest patches as well as sound and cursor themes at
 *  http://triq.net/icewm/
 *
 * Sound output interface types and notes:
 *
 *      OSS     Open Sound Source (generic)
 *      Y       Y sound systems
 *      ESounD  Enlightenment Sound Daemon (uses caching and *throttling)
 *
 * `*' Throttling is max. one sample every 500ms. Comments?
 */
#include "config.h"
#include "intl.h"

#if 1
#define THROW(Result) { rc = (Result); goto exceptionHandler; }
#define TRY(Command) { if ((rc = (Command))) THROW(rc); }
#define CATCH(Handler) { exceptionHandler: { Handler } return rc; }
#endif

#ifndef CONFIG_GUIEVENTS
#error Configure with "--enable-guievents"
#else /* CONFIG_GUIEVENTS */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#define GUI_EVENT_NAMES
#include "guievent.h"

#include "base.h" /* strJoin */
#include "ycmdline.h"

/******************************************************************************/

char const * ApplicationName = "icesound";

#define OSS_DEFAULT_DEVICE "/dev/dsp"
#define YIFF_DEFAULT_SERVER "127.0.0.1:9433"

/******************************************************************************
 * IceSound application
 ******************************************************************************/

class IceSound : public YCommandLine {
public:
    IceSound(int & argc, char **& argv):
        YCommandLine(argc, argv), dpyname(NULL) {
            ApplicationName = my_basename(argv[0]);
        }

        static void printUsage();
        int run();

protected:
    virtual char getArgument(char const * const & arg, char const *& val);
    virtual int setOption(char const * arg, char opt, char const * val);
    virtual int setArgument(int, char const *) { return 0; }

public:
    static char const * samples;
    static bool verbose;
    static volatile bool running;

private:
    static void exit();
    static void exit(int sig);
    static void hup(int sig);
    static void chld(int sig);

    char const * dpyname;
    static class YAudioInterface * audio;
};

/******************************************************************************
 * A generic audio interface
 ******************************************************************************/

class YAudioInterface {
public:
    virtual ~YAudioInterface() {}
    virtual int init(int & argc, char **& argv) = 0;
    virtual void play(int sid) = 0;
    virtual void reload() {}
    virtual void idle() {}

    /**
     * Finds a filename for sample with the specified id.
     * Returns NULL on error or the the full path to the sample file.
     * The string returned has to be freed by the caller
     */

    char * findSample(int sid)  {
        char basefname[1024];

        strcpy(basefname, gui_events[sid].name);
        strcat(basefname, ".wav");

        return findSample(basefname);
    }

    char * findSample(char const * basefname);
};

/**
 * Finds the sample specified.
 * Returns NULL on error or the the full path to the sample file.
 * The string returned has to be freed by the caller
 */
char * YAudioInterface::findSample(char const * basefname) {
    static char const * paths[] = {
        IceSound::samples,
        cstrJoin(getenv("HOME"), "/.icewm/sounds/", NULL),
        cstrJoin(CFGDIR, "/sounds/", NULL),
        cstrJoin(LIBDIR, "/sounds/", NULL)
    };

    for(unsigned i(0); i < ACOUNT(gui_events); i++)
        for (unsigned n(0); n < ACOUNT(paths); ++n)
            if(paths[n] != NULL) {
                char * filename = cstrJoin(paths[n], basefname, NULL);

                if (access(filename, R_OK) == 0)
                    return filename;

                delete[] filename;
            }

    return NULL;
}

/******************************************************************************
 * General (OSS) audio interface
 ******************************************************************************/

class YOSSAudio : public YAudioInterface {
public:
    YOSSAudio(): device(OSS_DEFAULT_DEVICE) {}

    virtual void play(int sound);
    virtual int init(int & argc, char **& argv);

private:
    friend class CommandLine;
    class CommandLine : public YCommandLine {
    public:
        CommandLine(int & argc, char **& argv, YOSSAudio & oss):
            YCommandLine(argc, argv), oss(oss) {}

            virtual char getArgument(char const * const & arg, char const *& val) {
                char const * larg(arg[1] == '-' ? arg + 2 : arg + 1);

                if (!strpcmp(larg, "device")) {
                    val = getValue(arg, strchr(arg, '='));
                    return 'D';
                }

                if (strchr("D", arg[1])) {
                    val = getValue(arg, arg[2] ? arg + 2 : NULL);
                    return arg[1];
                }

                return '\0';
            }

            virtual int setOption(char const * arg, char opt, char const * val) {
                switch(opt) {
                case 'D': // ======================================== device ===
                    oss.device = val;
                    return 0;

                default:
                    return YCommandLine::setOption(arg, opt, val);
                }
            }

    protected:
        YOSSAudio & oss;
    };

    friend class CommandLine;
    char const * device;
};

/**
 * Play a sound sample directly to the digital signal processor.
 */
void YOSSAudio::play(int sound) {
    if (device == NULL) return;

    for(unsigned i(0); i < ACOUNT(gui_events); i++)
        if(gui_events[i].type == sound) {
            char * samplefile(findSample(i));

#ifndef DEBUG
            if (IceSound::verbose)
#endif
                msg(_("Playing sample #%d (%s)"), sound, samplefile);

            if (samplefile) {
                int ifd(open(samplefile, O_RDONLY));

                if(ifd == -1) {
                    warn("%s: %s", samplefile, strerror(errno));
                    return;
                }

                int ofd(open(device, O_WRONLY));

                if(ofd == -1) {
                    warn("%s: %s", device, strerror(errno));
                    close(ifd);
                    return;
                }

                if (IceSound::verbose)
                    msg("TODO: adjust audio format"); // !!!

#ifdef DEBUG
                msg("copying sound %s to %s\n", samplefile, device);
#endif
                delete[] samplefile;

                char sbuf[4096];
                for (int n; (n = read(ifd, sbuf, sizeof(sbuf))) > 0; )
                    write(ofd, sbuf, n);

                close(ofd);
                close(ifd);
            }
        }
}

int YOSSAudio::init(int & argc, char **& argv) {
    int rc(0);

    TRY(CommandLine(argc, argv, *this).parse())

        if (access(device, W_OK) != 0) {
            warn(_("No such device: %s"), device);
            THROW(3)
        }

    CATCH(/**/)
}

/******************************************************************************
 * Enlightenment Sound Daemon audio interface
 ******************************************************************************/

#ifdef ENABLE_ESD

#include <esd.h>

class YESDAudio : public YAudioInterface {
public:
    YESDAudio():
        speaker(getenv("ESPEAKER")), socket(-1) {
            for (unsigned i = 0; i < ACOUNT(sample); ++i) sample[i] = -1;
        }

        virtual ~YESDAudio() {
            unloadSamples();
            if (socket > -1) esd_close(socket);
        }

        virtual void play(int sound);

        virtual void reload() {
            unloadSamples();
            uploadSamples();
        }

private:
    unsigned uploadSamples();
    unsigned unloadSamples();

    int uploadSample(int sound, char const * path);
    virtual int init(int & argc, char **& argv);

private:
    friend class CommandLine;
    class CommandLine : public YCommandLine {
    public:
        CommandLine(int & argc, char **& argv, YESDAudio & esd):
            YCommandLine(argc, argv), esd(esd) {}

            virtual char getArgument(char const * const & arg, char const *& val) {
                char const * larg(arg[1] == '-' ? arg + 2 : arg + 1);

                if (!(strpcmp(larg, "server") &&
                      strpcmp(larg, "speaker"))) {
                    val = getValue(arg, strchr(arg, '='));
                    return 'S';
                }

                if (strchr("S", arg[1])) {
                    val = getValue(arg, arg[2] ? arg + 2 : NULL);
                    return arg[1];
                }

                return '\0';
            }

            virtual int setOption(char const * arg, char opt, char const * val) {
                switch(opt) {
                case 'S': // ======================================= speaker ===
                    esd.speaker = val;
                    return 0;

                default:
                    return YCommandLine::setOption(arg, opt, val);
                }
            }

    private:
        YESDAudio & esd;
    };

protected:
    friend class CommandLine;
    char const * speaker;
    int sample[ACOUNT(gui_events)];     // cache sample ids
    int socket;                         // socket to ESound Daemon
};

int YESDAudio::init(int & argc, char **& argv) {
    int rc(0);

    TRY(CommandLine(argc, argv, *this).parse())

        if ((socket = esd_open_sound(speaker)) == -1) {
            warn(_("Can't connect to ESound daemon: %s"),
                 speaker ? speaker : _("<none>"));
            THROW(3);
        }

    CATCH(
          uploadSamples();
         )
}

/**
 * Upload a sample in the ESounD server.
 * Returns sample ID or negative on error.
 */

int YESDAudio::uploadSample(int sound, char const * path) {
    if(socket < 0) return -1;

    int rc(esd_file_cache(socket, ApplicationName, path));

    if(rc < 0)
        msg(_("Error <%d> while uploading `%s:%s'"), rc,
            ApplicationName, path);
    else {
        sample[sound] = rc;

        if (IceSound::verbose)
            msg(_("Sample <%d> uploaded as `%s:%s'"), rc,
                ApplicationName, path);
    }

    return rc;
}

/**
 * Unload all samples from the ESounD server.
 * Returns number of samples catched.
 */

unsigned YESDAudio::unloadSamples() {
    if(socket < 0) return 0;

    unsigned cnt(0);
    for(unsigned i(0); i < ACOUNT(gui_events); ++i)
        if(sample[i] > 0) {
            esd_sample_free(socket, sample[i]);
            cnt++;
        }

    return cnt;
}

/**
 * Upload all samples into the EsounD server.
 * Returns the number of loaded samples.
 */

unsigned YESDAudio::uploadSamples() {
    if(socket < 0) return 0;

    unsigned cnt(0);
    for(unsigned i(0); i < ACOUNT(gui_events); i++) {
        char * samplefile(findSample(i));

        if (samplefile != NULL) {
            if (uploadSample(i, samplefile) >= 0) ++cnt;
            delete[] samplefile;
        }
    }

    return cnt;
}

/**
 * Play a cached sound sample using ESounD.
 */

void YESDAudio::play(int sound) {
    if (socket < 0) return;

    for(unsigned i(0); i < ACOUNT(gui_events); i++)
        if(gui_events[i].type == sound) {

#ifndef DEBUG
            if (IceSound::verbose)
#endif
                msg(_("Playing sample #%d"), i);

            if(sample[i] > 0)
                esd_sample_play(socket, sample[i]);
        }
}

#endif /* ENABLE_ESD */

/* *************************** Y Section ************************** */
/* If ENABLE_YIFF is defined then the Y2 header files will be included
 * and the functions for Y server interfacing will be declared here
 * in this section.
 */
#ifdef ENABLE_YIFF

#include <Y2/Y.h>
#include <Y2/Ylib.h>

class YY2Audio : public YAudioInterface {
public:
    YY2Audio():
        server(NULL), recorder(getenv("RECORDER")),
        mode(NULL), matchMode(false) {
        }

        virtual ~YY2Audio() {
            if (server) YCloseConnection(server, False);
        }

        virtual void play(int sound);

        virtual int init(int & argc, char **& argv);

        virtual void idle();

        void listAudioModes();
        void play(char const * path,
                  Coefficient lVol = 1.0, Coefficient rVol = 1.0);

private:
    friend class CommandLine;
    class CommandLine : public YCommandLine {
    public:
        CommandLine(int & argc, char **& argv, YY2Audio & yiff):
            YCommandLine(argc, argv), yiff(yiff) {}

            virtual char getArgument(char const * const & arg, char const *& val) {
                char const * larg(arg[1] == '-' ? arg + 2 : arg + 1);

                if (!(strpcmp(larg, "server") &&
                      strpcmp(larg, "recorder"))) {
                    val = getValue(arg, strchr(arg, '='));
                    return 'S';
                } else if (!strpcmp(larg, "audio-mode")) {
                    val = getValue(arg, strchr(arg, '='));
                    return 'm';
                } else if (!strcmp(larg, "audio-mode-auto"))
                    return 'A';

                if (strchr("Srm", arg[1])) {
                    val = getValue(arg, arg[2] ? arg + 2 : NULL);
                    return arg[1];
                }

                return '\0';
            }

            virtual int setOption(char const * arg, char opt, char const * val);

    private:
        YY2Audio & yiff;
    };

protected:
    YConnection *server;                // Connection to Y server
    char const * recorder;              // Address:port string
    char const *mode;                   // Audio mode name
    bool matchMode;                     // Automantically adjust audio mode
};

int YY2Audio::init(int & argc, char **& argv) {
    YConnection * con(NULL);
    int rc(0);

    TRY(CommandLine(argc, argv, *this).parse())

        if (recorder == NULL) recorder = YIFF_DEFAULT_SERVER;
    con = YOpenConnection(NULL, recorder);

    if (NULL == con) {
        warn(_("Can't connect to YIFF server: %s"),
             recorder ? recorder : _("<none>"));
        THROW(3);
    }

    if(mode != NULL && YChangeAudioModePreset(con, mode))
        warn(_("Can't change to audio mode `%s'."), mode);

    /* Now set descriptor to server, this is incase a SIGHUP
     * or other async event occured during initialization. */
    server = con;

    CATCH(/**/)
}

/**
 * Prints list of preset Audio modes on the Y server.
 */
void YY2Audio::listAudioModes() {
    if (recorder == NULL) recorder = YIFF_DEFAULT_SERVER;
    YConnection * con(YOpenConnection(NULL, recorder));

    if (con) {
        YAudioModeValuesStruct ** modes;
        int num;

        if ((modes = YGetAudioModes(con, &num)) != NULL) {
            for (int i(0); i < num; ++i) {
                YAudioModeValuesStruct * m_ptr(modes[i]);

                if(m_ptr != NULL)
                    printf("%s: %i Hz, %i bits, %i ch\n",
                           m_ptr->name, m_ptr->sample_rate,
                           m_ptr->sample_size, m_ptr->channels);
            }

            YFreeAudioModesList(modes, num);
        }

        YCloseConnection(con, False);
    } else
        warn(_("Can't connect to YIFF server: %s"),
             recorder ? recorder : _("<none>"));
}

void YY2Audio::play(int sound) {
    for(unsigned i(0); i < ACOUNT(gui_events); i++)
        if(gui_events[i].type == sound) {
            char * samplefile(findSample(i));

            if (samplefile != NULL) {
#ifndef DEBUG
                if (IceSound::verbose)
#endif
                    msg(_("Playing sample #%d (%s)"), sound, samplefile);

                play(samplefile);
                delete[] samplefile;
            }
        }
}

/**
 * Plays the specified path on the Y server.
 */
void YY2Audio::play(char const * path, Coefficient lVol, Coefficient rVol) {
    if(server == NULL || path == NULL) return;

    lVol = max(lVol, 0.0);
    rVol = max(rVol, 0.0);

    if(matchMode) {
        YEventSoundObjectAttributes sAttr;
        YAudioModeValuesStruct **modes;
        int mCount;

        if(!YGetSoundObjectAttributes(server, path, &sAttr) &&
           (modes = YGetAudioModes(server, &mCount)) != NULL) {
            YAudioModeValuesStruct *matchingMode(NULL);

            for(int i(0); i < mCount && matchingMode == NULL; ++i) {
                YAudioModeValuesStruct *mPtr(modes[i]);

                if (mPtr &&
                    mPtr->sample_rate == sAttr.sample_rate &&
                    mPtr->channels == sAttr.channels &&
                    mPtr->sample_size == sAttr.sample_size)
                    matchingMode = mPtr;
            }

            if(matchingMode)
                YChangeAudioModePreset(server, matchingMode->name);
            else {
                /* No Audio mode matched, try to set explicit Audio
                 * values. */

                long cycle(max(100L, YCalculateCycle
                               (server, sAttr.sample_rate, sAttr.channels,
                                sAttr.sample_size, 4096)));

                YSetAudioModeValues(server, sAttr.sample_rate, sAttr.channels,
                                    sAttr.sample_size,
                                    0, 1, 2, 4096);
                YSyncAll(server, True);
                YSetCycle(server, cycle);
            }

            YFreeAudioModesList(modes, mCount);
        }
    }

    YEventSoundPlay value;
    value.flags = YPlayValuesFlagPosition |
        YPlayValuesFlagTotalRepeats |
        YPlayValuesFlagVolume;

    value.position = 0;                 /* `From the top' */
    value.total_repeats = 1;            /* Repeat once. */
    value.left_volume = lVol;
    value.right_volume = rVol;

    YStartPlaySoundObject(server, path, &value);
}

void YY2Audio::idle() {
    YEvent yev;

    while(YGetNextEvent(server, &yev, False) > 0) {
        switch(yev.type) {
        case YAudioChange:
#ifdef DEBUG
            if(yev.audio.preset)
                msg("Y server switched to preset Audio `%s'",
                    yev.audio.mode_name);
            else
                msg("Y server changed Audio to: "
                    "%i Hz, %i bits, %i ch, %i bytes\n",
                    yev.audio.sample_rate,
                    yev.audio.sample_size,
                    yev.audio.channels,
                    yev.audio.fragment_size_bytes);
#endif
            /* If we are automatically adjusting the Audio mode and some
             * other Y client or the Y server itself has changed the Audio then
             * we should stop automatic changing of Audio mode to prevent
             * `fighting' with the other Y client or Y server. */

            if (mode) {
                msg(_("Audio mode switch detected, "
                      "initial audio mode `%s' no longer in effect."),
                    mode);
                mode = NULL;
            }

            if (matchMode) {
                msg(_("Audio mode switch detected, "
                      "automatic audio mode changing disabled."));
                matchMode = false;
            }

        case YSoundObjectKill:
#ifdef DEBUG
            msg("Y sound object #%ld finished playing.", yev.kill.yid);
#endif
            break;

        case YDisconnect:
#ifdef DEBUG
            msg("Lost connection to Y server, reason `%d'\n",
                yev.disconnect.reason);
#endif
            YCloseConnection(server, False);
            server = NULL;
            break;

        case YShutdown:
#ifdef DEBUG
            msg("Y server has shutdown, reason `%i'\n",
                yev.shutdown.reason);
#endif
            YCloseConnection(server, False);
            server = NULL;
            break;
        }

        /* Has the Y server been detected to disconnected us or
         * shutdown in the above event handling? */

        IceSound::running = (server != NULL);
    }
}

int YY2Audio::CommandLine::setOption(char const * arg, char opt, char const * val) {
    switch(opt) {
    case 'S': // ====================================== recorder ===
    case 'r':
        yiff.recorder = val;
        return 0;

    case 'm': // ==================================== audio-mode ===
        if (yiff.mode != NULL)
            warn(_("Overriding previous audio mode `%s'."),
                 yiff.mode);

        if (val == NULL || strcmp(val, "?") == 0) {
            yiff.listAudioModes();
            return -1;
        }

        yiff.mode = val;
        return 0;

    case 'A': // =============================== audio-mode-auto ===
        if (yiff.mode != NULL)
            warn(_("Overriding previous audio mode `%s'."),
                 yiff.mode);

        yiff.matchMode = true;
        return 0;

    default:
        return YCommandLine::setOption(arg, opt, val);
    }
}

#endif  /* ENABLE_YIFF */

/******************************************************************************
 * IceSound application
 ******************************************************************************/

char const * IceSound::samples(NULL);
volatile bool IceSound::running(true);
bool IceSound::verbose(false);
YAudioInterface * IceSound::audio(NULL);

/**
 * Print usage information for icesound
 */
void IceSound::printUsage() {
    printf(_("\
             Usage: %s [OPTION]...\n\
             \n\
             Plays audio files on GUI events raised by IceWM.\n\
             \n\
             Options:\n\
             \n\
             -d, --display=DISPLAY         Display used by IceWM (default: $DISPLAY).\n\
             -s, --sample-dir=DIR          Specifies the directory which contains\n\
             the sound files (ie ~/.icewm/sounds).\n\
             -i, --interface=TARGET        Specifies the sound output target\n\
             interface, one of OSS, YIFF, ESD\n\
             -D, --device=DEVICE           (OSS only) specifies the digital signal\n\
             processor (default /dev/dsp).\n\
             -S, --server=ADDR:PORT     (ESD and YIFF) specifies server address and\n\
             port number (default localhost:16001 for ESD\n\
             and localhost:9433 for YIFF).\n\
             -m, --audio-mode[=MODE]       (YIFF only) specifies the Audio mode (leave\n\
             blank to get a list).\n\
             --audio-mode-auto          (YIFF only) change Audio mode on the fly to\n\
             best match sample's Audio (can cause\n\
             problems with other Y clients, overrides\n\
             --audio-mode).\n\
             \n\
             -v, --verbose                 Be verbose (prints out each sound event to\n\
             stdout).\n\
             -V, --version                 Prints version information and exits.\n\
             -h, --help                    Prints (this) help screen and exits.\n\
             \n\
             Return values:\n\
             \n\
             0     Success.\n\
             1     General error.\n\
             2     Command line error.\n\
             3     Subsystems error (ie cannot connect to server).\n\n"),
           ApplicationName);

    return;
}

/**
 * The hearth of icesound
 */
int IceSound::run() {
    int rc(0);

    Display * display(NULL);
    Window root(None);
    Atom _GUI_EVENT(None);

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

#ifdef DEBUG
    msg("Compiled with DEBUG flag. Debugging messages will be printed.");
#endif

    TRY(parse())

        signal(SIGINT, exit); // ============================= ensure clean exit ===
    signal(SIGTERM, exit);
    signal(SIGPIPE, exit);

    if (audio == NULL) audio = new YOSSAudio(); // ==== init audio interface ===
    if ((rc = audio->init(argc, argv))) {
        printf("%d\n", rc);
        THROW(max(rc, 0))
    }

    if(NULL == (display = XOpenDisplay(dpyname))) { // ====== connect to X11 ===
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(dpyname));
        THROW(3)
    }

    root = RootWindow(display, DefaultScreen(display));
    _GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);
    XSelectInput(display, root, PropertyChangeMask);

    signal(SIGCHLD, chld); // ================================= IPC handlers ===
    signal(SIGHUP, hup);

    struct timeval last; // ================================= X message loop ===
    gettimeofday(&last, NULL);

    while(running) {
        audio->idle();

        while (XPending(display)) {
            XEvent xev;

            XNextEvent(display, &xev);
            switch(xev.type) {
            case PropertyNotify:
                if (xev.xproperty.atom == _GUI_EVENT &&
                    xev.xproperty.state == PropertyNewValue) {
                    Atom type; int format;
                    unsigned long nitems, lbytes;
                    unsigned char *propdata;
                    int gev = -1;

                    if (XGetWindowProperty(display, root, _GUI_EVENT,
                                           0, 3, False, _GUI_EVENT,
                                           &type, &format, &nitems, &lbytes,
                                           &propdata) == Success &&
                        propdata) {
                        gev = *(char *) propdata;
                        XFree(propdata);
                    }

                    /* Recieved restart event? */
                    if(geRestart == gev) { // ------------ restart event ---
#ifdef DEBUG
                        msg("Restart event received");
#endif
                        hup(SIGHUP);
                    }

                    struct timeval now; // ---------------- update timing ---
                    gettimeofday(&now, NULL);

                    if((now.tv_sec - last.tv_sec) >= 2 ||
                       ((now.tv_sec - last.tv_sec) * 1000000 +
                        now.tv_usec - last.tv_usec) > 500000) {
                        last = now;
                        audio->play(gev);
                    }
                }
                break;
            }
        }

        usleep(10000);
    }

    CATCH(
          if (display) XCloseDisplay(display);
          if (audio) delete audio;
         )
}

char IceSound::getArgument(char const * const & arg, char const *& val) {
    char const * larg(arg[1] == '-' ? arg + 2 : arg + 1);

    if (!strpcmp(larg, "help"))
        return 'h';
    else if (!strcmp(larg, "verbose"))
        return 'v';
    else if (!strcmp(larg, "version"))
        return 'V';
    else if (!strpcmp(larg, "display")) {
        val = getValue(arg, strchr(arg, '='));
        return 's';
    } else if (!strpcmp(larg, "sample-dir")) {
        val = getValue(arg, strchr(arg, '='));
        return 's';
    } else if (!strpcmp(larg, "interface")) {
        val = getValue(arg, strchr(arg, '='));
        return 'i';
    }

    if (strchr("dsi", arg[1])) {
        val = getValue(arg, arg[2] ? arg + 2 : NULL);
        return arg[1];
    }

    return (strchr("h?vV", arg[1]) ? arg[1] : '\0');
}

int IceSound::setOption(char const * /*arg*/, char opt, char const * val) {
    switch(opt) {
    case '?': // ================================================ --help ===
    case 'h':
        printUsage();
        return 2;

    case 'V': // ============================================= --version ===
        puts(VERSION);
        return -1;

    case 'v': // ============================================= --verbose ===
        verbose = true;
        break;

    case 'd': // ============================================= --display ===
        dpyname = val;
        break;

    case 's': // ========================================== --sample-dir ===
        samples = val;
        break;

    case 'i': // ========================================== --interface ====
        if(val != NULL) {
            if (audio != NULL)
                warn(_("Multiple sound interfaces given."));

            if(!(strcmp(val, "OSS") &&
                 strcmp(val, "oss"))) {
                delete audio;
                audio = new YOSSAudio();
            } else if(!(strcmp(val, "ESD") &&
                        strcmp(val, "esd") &&
                        strcmp(val, "ESOUND") &&
                        strcmp(val, "esound") &&
                        strcmp(val, "ESounD"))) {
#ifdef ENABLE_ESD
                delete audio;
                audio = new YESDAudio();
#else
                warn(_("Support for the %s interface not compiled."), val);
                return 2;
#endif
            } else if(!(strcmp(val, "Y") &&
                        strcmp(val, "y") &&
                        strcmp(val, "Y2") &&
                        strcmp(val, "y2") &&
                        strcmp(val, "YIFF") &&
                        strcmp(val, "yiff"))) {
#ifdef ENABLE_YIFF
                delete audio;
                audio = new YY2Audio();
#else
                warn(_("Support for the %s interface not compiled."), val);
                return 2;
#endif
            } else {
                warn(_("Unsupported interface: %s."), val);
                return 2;
            }
        }

        break;
    }

    return 0;
}

/**
 * Signal handler.
 */
void IceSound::exit(int sig) {
    signal(sig, SIG_DFL);

    msg(_("Received signal %d: Terminating..."), sig);
    running = false;
}

/**
 * SIGHUP signal handler, restarts and reloads data.
 */
void IceSound::hup(int sig) {
    if (sig == SIGHUP) {
        msg(_("Received signal %d: Reloading samples..."), sig);
        if (audio) audio->reload();
    } else
        msg("Internal error: Received signal %i is not SIGHUP", sig);
}

/**
 * SIGCHLD signal handler.
 */
void IceSound::chld(int sig) {
    if(sig == SIGCHLD) {
        pid_t pid; int stat;

        while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
#ifdef DEBUG
            msg("Child %d terminated", pid);
#endif
        }
    } else
        msg("Internal error: Received signal %i is not SIGCHLD", sig);
}

int main(int argc, char *argv[]) {
    return IceSound(argc, argv).run();
}

#endif
