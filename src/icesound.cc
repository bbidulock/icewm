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
 * 2005-11-30:  Christian W. Zuckschwerdt  <zany@triq.net>
 *              Added ALSA interface support
 * TODO: a virtual YAudioInterface class the OSS, ESD and YIFF implement
 *
 * for now get latest patches as well as sound and cursor themes at
 *  http://triq.net/icewm/
 *
 * Sound output interface types and notes:
 *
 *      ALSA    Advanced Linux Sound Architecture (needs libsndfile)
 *      OSS     Open Sound Source (generic)
 *      Y       Y sound systems
 *      ESounD  Enlightenment Sound Daemon (uses caching and *throttling)
 *
 * `*' Throttling is max. one sample every 500ms. Comments?
 */
#include "config.h"
#include "ytimer.h"
#include "base.h"
#include "ypointer.h"
#include "upath.h"
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
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <X11/Xlib.h>

#define GUI_EVENT_NAMES
#include "guievent.h"


/******************************************************************************/

char const * ApplicationName = "icesound";

#define ALSA_DEFAULT_DEVICE "default"
#define OSS_DEFAULT_DEVICE "/dev/dsp"

static const char audio_interfaces[] =
#ifdef ENABLE_ESD
    "ESD,"
#endif
#ifdef ENABLE_ALSA
    "ALSA,"
#endif
    "OSS";

/******************************************************************************
 * IceSound application
 ******************************************************************************/

class SoundConf {
public:
    virtual ~SoundConf() {}
    virtual bool verbose() const = 0;
    virtual const char* alsaDevice() const = 0;
    virtual const char* ossDevice() const = 0;
    virtual const char* esdServer() const = 0;

    /**
     * Finds a filename for sample with the specified gui event.
     * Returns NULL on error or the full path to the sample file.
     * The string returned has to be freed by the caller
     */
    virtual char* findSample(int sound) const = 0;
};

static class SoundAsync {
public:
    volatile bool running;
    volatile bool reload;
} soundAsync = { true, false };

class IceSound : public SoundConf {
public:
    IceSound(int argc, char** argv);
    virtual ~IceSound() {}

    virtual bool verbose() const { return verbosity; }
    virtual const char* alsaDevice() const {
        return alsaDeviceFile ? alsaDeviceFile :
            deviceFile ? deviceFile : ALSA_DEFAULT_DEVICE;
    }
    virtual const char* ossDevice() const {
        return ossDeviceFile ? ossDeviceFile :
            deviceFile ? deviceFile : OSS_DEFAULT_DEVICE;
    }
    virtual const char* esdServer() const {
        return esdServerName ? esdServerName : getenv("ESPEAKER");
    }
    virtual char* findSample(int sound) const;

    int run();
    static void printUsage();

private:
    bool verbosity;
    char const* sampleDir;
    char const* deviceFile;
    char const* alsaDeviceFile;
    char const* ossDeviceFile;
    char const* esdServerName;
    char const* displayName;
    char const* interfaceNames;
    class YAudioInterface* audio;
    upath paths[6];
    Atom _GUI_EVENT;
    Display* display;
    Window root;
    timeval last;

    void initPaths();
    int chooseInterface();
    void readEvents();
    void guiEvent();
    int getProperty();

private:
    static void stop(int sig);
    static void hup(int sig);
    static void chld(int sig);
};

/******************************************************************************
 * A generic audio interface
 ******************************************************************************/

class YAudioInterface {
public:
    virtual ~YAudioInterface() {}
    virtual int init(SoundConf* conf) = 0;
    virtual void play(int sound) = 0;
    virtual void reload() {}
};

/******************************************************************************
 * ALSA audio interface
 ******************************************************************************/

#ifdef ENABLE_ALSA

#include <alsa/asoundlib.h>
#if HAVE_SNDFILE_H
#include <sndfile.h>
#else
#error Please install libsndfile-dev
#endif

class YALSAAudio : public YAudioInterface {
public:
    YALSAAudio();
    virtual ~YALSAAudio();

    virtual void play(int sound);
    virtual int init(SoundConf* conf) {
        this->conf = conf;
        return open();
    }

private:
    int open();

    SoundConf* conf;
    snd_pcm_t *playback_handle;
};

YALSAAudio::YALSAAudio() :
    conf(0),
    playback_handle(0)
{
}

YALSAAudio::~YALSAAudio() {
    if (playback_handle) {
        snd_pcm_hw_free(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = 0;
    }
}

int YALSAAudio::open() {
    int err;
    if ((err = snd_pcm_open(&playback_handle,
                    conf->alsaDevice(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        warn("cannot open audio device %s (%s)\n",
                 conf->alsaDevice(),
                 snd_strerror(err));
        return 3;
    }
    return 0;
}

/**
 * Play a sound sample directly to the digital signal processor.
 */
void YALSAAudio::play(int sound) {
    if (conf == NULL || playback_handle == NULL) {
        tlog("ALSA NULL");
        return;
    }

    csmart samplefile(conf->findSample(sound));
    if (samplefile == NULL)
        return;

    if (conf->verbose())
        tlog(_("Playing sample #%d (%s)"), sound, (char *) samplefile);

    int err;
    snd_pcm_hw_params_t *hw_params(0);

    SF_INFO sfinfo = {};
    SNDFILE* sf = sf_open(samplefile, SFM_READ, &sfinfo);
    if (sf == NULL) {
        warn("%s: %s", (char *) samplefile, sf_strerror(sf));
        goto done;
    }

    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        warn("cannot allocate hardware parameter structure (%s)\n",
                 snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
        warn("cannot initialize hardware parameter structure (%s)\n",
                 snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params,
                    SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        warn("cannot set access type (%s)\n", snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
                    SND_PCM_FORMAT_S16_LE)) < 0) {
        warn("cannot set sample format (%s)\n", snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params,
                    (unsigned int *)&sfinfo.samplerate, 0)) < 0) {
        warn("cannot set sample rate (%s)\n", snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params,
                    sfinfo.channels)) < 0) {
        warn("cannot set channel count (%s)\n", snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
        warn("cannot set parameters (%s)\n", snd_strerror(err));
        goto done;
    }

    if ((err = snd_pcm_prepare(playback_handle)) < 0) {
        warn("cannot prepare audio interface (%s)\n", snd_strerror(err));
        goto done;
    }

    {
        const int N = 8*1024;
        short sbuf[N]; // period_size * channels * snd_pcm_format_width(format)) / 8
        for (int n; (n = sf_readf_short(sf, sbuf, N / 2)) > 0; ) {
            if ((err = snd_pcm_writei(playback_handle, sbuf, n) != n)) {
                warn("write to audio interface failed (%s) %zd %d\n",
                         snd_strerror(err), sizeof(short), n);
                goto done;
            }
        }
    }

done:
    snd_pcm_hw_params_free(hw_params);
    sf_close(sf);
}

#endif /* ENABLE_ALSA */

/******************************************************************************
 * General (OSS) audio interface
 ******************************************************************************/

class YOSSAudio : public YAudioInterface {
public:
    YOSSAudio(): conf(0) {}

    virtual void play(int sound);
    virtual int init(SoundConf* conf);

private:
    SoundConf* conf;
};

/**
 * Play a sound sample directly to the digital signal processor.
 */
void YOSSAudio::play(int sound) {
    if (conf == NULL) {
        tlog("OSS NULL");
        return;
    }

    csmart samplefile(conf->findSample(sound));
    if (samplefile == NULL)
        return;

    if (conf->verbose())
        tlog(_("Playing sample #%d (%s)"), sound, (char *) samplefile);

    int ifd(open(samplefile, O_RDONLY));
    if (ifd == -1) {
        fail("%s", (char *) samplefile);
        return;
    }

    int ofd(open(conf->ossDevice(), O_WRONLY));
    if (ofd == -1) {
        fail("%s", conf->ossDevice());
        close(ifd);
        return;
    }

    // TODO: adjust audio format !!!
    if (conf->verbose())
        tlog("TODO: adjust audio format"); // !!!

    MSG(("copying sound %s to %s\n", samplefile, device));

    char sbuf[4096];
    for (int n; (n = read(ifd, sbuf, sizeof(sbuf))) > 0; )
        if (write(ofd, sbuf, n) != n)
            if (n != -1 || errno != EINTR)
                break;

    close(ofd);
    close(ifd);
}

int YOSSAudio::init(SoundConf* conf) {
    this->conf = conf;
    if (access(conf->ossDevice(), W_OK) != 0) {
        fail(_("%s"), conf->ossDevice());
        return 3;
    }
    return 0;
}

/******************************************************************************
 * Enlightenment Sound Daemon audio interface
 ******************************************************************************/

#ifdef ENABLE_ESD

#include <esd.h>

class YESDAudio : public YAudioInterface {
public:
    YESDAudio():
        socket(-1)
    {
        for (unsigned i = 0; i < ACOUNT(sample); ++i) sample[i] = -1;
    }

    virtual ~YESDAudio() {
        unloadSamples();
        if (socket > -1) {
            esd_close(socket);
            socket = -1;
        }
    }

    virtual void play(int sound);

    virtual void reload() {
        unloadSamples();
        uploadSamples();
    }

    virtual int init(SoundConf* conf);

private:
    int uploadSamples();
    int unloadSamples();

    int uploadSample(int sound, char const * path);

protected:
    SoundConf* conf;
    int sample[NUM_GUI_EVENTS];     // cache audio samples
    int socket;                     // socket to ESound Daemon
};

int YESDAudio::init(SoundConf* conf) {
    this->conf = conf;

    const char* server = conf->esdServer();
    if ((socket = esd_open_sound(server)) == -1) {
        warn(_("Can't connect to ESound daemon: %s"),
             server ? server : _("<none>"));
        return 3;
    }

    uploadSamples();
    return 0;
}

/**
 * Upload a sample in the ESounD server.
 * Returns sample ID or negative on error.
 */
int YESDAudio::uploadSample(int sound, char const * path) {
    if (socket < 0) return -1;

    int rc(esd_file_cache(socket, ApplicationName, path));

    if (rc < 0)
        msg(_("Error <%d> while uploading `%s:%s'"), rc,
            ApplicationName, path);
    else {
        sample[sound] = rc;

        if (conf->verbose())
            tlog(_("Sample <%d> uploaded as `%s:%s'"), rc,
                ApplicationName, path);
    }

    return rc;
}

/**
 * Unload all samples from the ESounD server.
 * Returns number of samples catched.
 */

int YESDAudio::unloadSamples() {
    if (socket < 0) return 0;

    int cnt(0);
    for (int i(0); i < NUM_GUI_EVENTS; ++i)
        if (sample[i] > 0) {
            esd_sample_free(socket, sample[i]);
            cnt++;
        }

    return cnt;
}

/**
 * Upload all samples into the EsounD server.
 * Returns the number of loaded samples.
 */

int YESDAudio::uploadSamples() {
    if (socket < 0) return 0;

    int cnt(0);
    for (int i(0); i < NUM_GUI_EVENTS; i++) {
        csmart samplefile(conf->findSample(i));

        if (samplefile != NULL) {
            if (uploadSample(i, samplefile) >= 0) ++cnt;
        }
    }

    if (soundAsync.reload)
        soundAsync.reload = false;

    return cnt;
}

/**
 * Play a cached sound sample using ESounD.
 */
void YESDAudio::play(int sound) {
    if (socket < 0) return;

    if (soundAsync.reload)
        reload();

    if (conf->verbose())
        tlog(_("Playing sample #%d: %d"), sound, sample[sound]);

    if (sample[sound] > 0)
        esd_sample_play(socket, sample[sound]);
}

#endif /* ENABLE_ESD */

/******************************************************************************
 * IceSound application
 ******************************************************************************/

IceSound::IceSound(int argc, char** argv) :
    verbosity(false),
    sampleDir(0),
    deviceFile(0),
    alsaDeviceFile(0),
    ossDeviceFile(0),
    esdServerName(0),
    displayName(0),
    interfaceNames(0),
    audio(0),
    _GUI_EVENT(None),
    display(NULL),
    root(None),
    last(monotime())
{
#ifdef DEBUG
    verbosity = true;
#endif
    initPaths();
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char* value(0);
            if (GetArgument(value, "d", "display", arg, argv + argc)) {
                displayName = value;
            }
            else if (GetArgument(value, "s", "sample-dir", arg, argv + argc)) {
                sampleDir = value;
            }
            else if (GetArgument(value, "i", "interface", arg, argv + argc)) {
                interfaceNames = value;
            }
            else if (GetArgument(value, "D", "device", arg, argv + argc)) {
                deviceFile = value;
            }
            else if (GetArgument(value, "A", "alsa", arg, argv + argc)) {
                alsaDeviceFile = value;
            }
            else if (GetArgument(value, "O", "oss", arg, argv + argc)) {
                ossDeviceFile = value;
            }
            else if (GetArgument(value, "S", "server", arg, argv + argc)) {
                esdServerName = value;
            }
            else if (is_switch(*arg, "v", "verbose")) {
                verbosity = true;
            }
            else if (is_help_switch(*arg)) {
                printUsage();
            }
            else if (is_version_switch(*arg)) {
                print_version_exit(VERSION);
            }
            else {
                warn(_("Unrecognized option: %s\n"), *arg);
            }
        } else {
            warn(_("Unrecognized argument: %s\n"), *arg);
        }
    }
}

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
 -d, --display=DISPLAY      X11 display used by IceWM (default: $DISPLAY).\n\
\n\
 -s, --sample-dir=DIR       Specifies the directory which contains the sound\n\
                            files (default is $HOME/.config/icewm/sounds).\n\
\n\
 -i, --interface=TARGET(S)  Specifies the sound output target interface(s).\n\
                            One or more of ESD, ALSA, OSS separated by commas.\n\
\n\
 -D, --device=DEVICE        Backwards compatibility only: the default device. \n\
                            Please prefer one of the -A/-O/-S options.\n\
\n\
 -O, --oss=DEVICE           Specifies the OSS device (default: \"%s\").\n\
\n\
 -A, --alsa=DEVICE          Specifies the ALSA device (default: \"%s\").\n\
\n\
 -S, --server=ADDR:PORT     Specifies the ESD server address and port number.\n\
                            For ESD the default is \"localhost:16001\".\n\
\n\
 -v, --verbose              Be verbose and print out each sound event.\n\
 -V, --version              Prints version information and exits.\n\
 -h, --help                 Prints this help screen and exits.\n\
\n\
Return values:\n\
\n\
  0    Success.\n\
  1    General error.\n\
  2    Command line error.\n\
  3    Subsystems error (ie cannot connect to server).\n\n"),
           ApplicationName, ALSA_DEFAULT_DEVICE, OSS_DEFAULT_DEVICE);

    ::exit(0);
}

void IceSound::initPaths() {
    int k = 0;
    if (sampleDir) {
        paths[k++] = upath(sampleDir) + "/";
    }
    const char* home = getenv("HOME");
    if (home) {
        upath uhome(home);
        paths[k++] = uhome + "/.config/icewm/sounds/";
        paths[k++] = uhome + "/.icewm/sounds/";
    }
    paths[k++] = CFGDIR "/sounds/";
    paths[k++] = LIBDIR "/sounds/";
    assert(k < (int) ACOUNT(paths));
}

/**
 * Finds a filename for sample with the specified gui event.
 * Returns NULL on error or the full path to the sample file.
 * The string returned has to be freed by the caller
 */
char* IceSound::findSample(int sound) const {
    if (inrange(sound, 0, NUM_GUI_EVENTS - 1)) {
        upath base(mstring(gui_event_names[sound]) + ".wav");
        for (int k = 0; paths[k] != null; ++k) {
            upath file(paths[k] + base);
            if (file.isReadable()) {
                return newstr(file.string());
            }
        }
        if (verbose())
            tlog(_("No audio for %s"), gui_event_names[sound]);
    }
    return NULL;
}

/**
 * The hearth of icesound
 */
int IceSound::run() {
    int rc(0);

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    MSG(("Compiled with DEBUG flag. Debugging messages will be printed."));

    signal(SIGINT, stop); // === ensure clean exit ===
    signal(SIGTERM, stop);
    signal(SIGPIPE, stop);

    TRY(chooseInterface());

    if (NULL == (display = XOpenDisplay(displayName))) { // === connect to X11 ===
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(displayName));
        THROW(3)
    }

    root = RootWindow(display, DefaultScreen(display));
    _GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);
    XSelectInput(display, root, PropertyChangeMask);

    signal(SIGCHLD, chld); // === IPC handlers ===
    signal(SIGHUP, hup);

    while (soundAsync.running) {
        readEvents();
        usleep(10000);
    }

    CATCH(
        if (display) {
            XCloseDisplay(display);
            display = NULL;
        }
        if (audio) {
            delete audio;
            audio = NULL;
        }
    )
}

void IceSound::readEvents() {
    while (XPending(display)) {
        XEvent xev;
        xev.type = 0;
        XNextEvent(display, &xev);
        if (xev.type == PropertyNotify &&
            xev.xproperty.atom == _GUI_EVENT &&
            xev.xproperty.state == PropertyNewValue)
        {
            guiEvent();
        }
    }
}

int IceSound::getProperty() {
    Atom type;
    int format;
    unsigned long nitems, lbytes;
    unsigned char *propdata(0);
    int gev(-1);

    if (XGetWindowProperty(display, root, _GUI_EVENT,
                           0, 3, False, _GUI_EVENT,
                           &type, &format, &nitems, &lbytes,
                           &propdata) == Success && propdata)
    {
        gev = propdata[0];
        XFree(propdata);
    }
    return gev;
}

void IceSound::guiEvent() {
    int gev = getProperty();
    if (gev < 0) {
        warn(_("Could not get GUI event property"));
        return;
    }
    if (gev >= NUM_GUI_EVENTS) {
        warn(_("Received invalid GUI event %d"), gev);
        return;
    }
    if (verbose())
        tlog(_("Received GUI event %s"), gui_event_names[gev]);

    /* Recieved restart event? */
    if (geRestart == gev) { // --- restart event ---
        MSG(("Restart event received"));
        soundAsync.reload = true;
    }

    timeval now = monotime();
    if (last + millitime(500L) < now) {
        last = now;
        audio->play(gev);
    }
    else if (verbose())
        tlog(_("Too quick; ignoring."));
}

int IceSound::chooseInterface() {
    int rc(3);
    mstring list(interfaceNames ? interfaceNames : audio_interfaces);
    mstring name;
    while (audio == NULL && list.splitall(',', &name, &list)) {
        if (name.isEmpty())
            continue;
        name = name.upper();
        const char* val = cstring(name);
        if (name == "OSS") {
            audio = new YOSSAudio();
        }
        else if (name == "ALSA") {
#ifdef ENABLE_ALSA
            audio = new YALSAAudio();
#else
            warn(_("Support for the %s interface not compiled."), val);
#endif
        }
        else if (name == "ESD" || name == "ESOUND") {
#ifdef ENABLE_ESD
            audio = new YESDAudio();
#else
            warn(_("Support for the %s interface not compiled."), val);
#endif
        }
        else {
            warn(_("Unsupported interface: %s."), val);
        }
        if (audio) {
            rc = audio->init(this);
            if (rc) {
                delete audio;
                audio = 0;
            }
        }
    }

    return rc;
}

/**
 * Signal handler.
 */
void IceSound::stop(int sig) {
    signal(sig, SIG_DFL);

    msg(_("Received signal %d: Terminating..."), sig);
    soundAsync.running = false;
}

/**
 * SIGHUP signal handler, restarts and reloads data.
 */
void IceSound::hup(int sig) {
    if (sig == SIGHUP) {
        // set the reload flag and let the driver check this.
        soundAsync.reload = true;
    }
}

/**
 * SIGCHLD signal handler.
 */
void IceSound::chld(int sig) {
    if (sig == SIGCHLD) {
        pid_t pid;
        do {
            pid = waitpid(-1, NULL, WNOHANG);
            if (pid > 0) {
                MSG(("Child %d terminated", pid));
            }
        } while (pid > 0 || (pid == -1 && errno == EINTR));
    }
}

int main(int argc, char *argv[]) {
    ApplicationName = my_basename(argv[0]);
    return IceSound(argc, argv).run();
}

#endif
