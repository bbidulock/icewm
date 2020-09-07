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
 * 2017-08-11:  Redesign by Bert Gijsbers. Fix ALSA. Remove YCommandLine.
 *              From multiple enabled interfaces choose the one that works.
 *              Replace the busy-wait polling with a blocking select.
 *              Added support for AO.
 *
 * for now get latest patches as well as sound and cursor themes at
 *  http://triq.net/icewm/
 *
 * Sound output interface types and notes:
 *
 *      ALSA    Advanced Linux Sound Architecture (needs libsndfile)
 *      AO      libao: a cross platform audio library (needs libsndfile)
 *      OSS     Open Sound Source (generic)
 *      ESounD  Enlightenment Sound Daemon (uses caching and *throttling)
 *
 * `*' Throttling is max. one sample every 500ms. Comments?
 */
#include "config.h"
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <X11/Xlib.h>

#include "ytime.h"
#include "base.h"
#include "ypointer.h"
#include "upath.h"
#include "intl.h"
#define GUI_EVENT_NAMES
#include "guievent.h"

#ifdef HAVE_SYS_IOCTL_H
#include <unistd.h>
#include <sys/ioctl.h>
#else
#undef ENABLE_OSS
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#undef ENABLE_OSS
#endif

#if defined(ENABLE_ALSA) || defined(ENABLE_AO) || defined(ENABLE_OSS)
#include <sndfile.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
// that's precise enough to detect a modern system
#include <sys/types.h>
#include <sys/stat.h>
#endif

/******************************************************************************/

char const * ApplicationName = "icesound";

#define ALSA_DEFAULT_DEVICE "default"
#define OSS_DEFAULT_DEVICE "/dev/dsp"
#define DEFAULT_SNOOZE_TIME 500L

enum IcesoundStatus {
    ICESOUND_SUCCESS  = 0,
    ICESOUND_IF_ERROR = 3,
};

static const char audio_interfaces[] =
#ifdef ENABLE_AO
    "AO,"
#endif
#ifdef ENABLE_ALSA
    "ALSA,"
#endif
#ifdef ENABLE_OSS
    "OSS"
#endif
    "";

static class SoundAsync {
public:
    volatile bool running;
    volatile bool reload;
} soundAsync = { true, false };

/******************************************************************************
 * A generic audio interface
 ******************************************************************************/

class SoundConf {
public:
    virtual ~SoundConf() {}
    virtual bool verbose() const = 0;
    virtual const char* alsaDevice() const = 0;
    virtual const char* ossDevice() const = 0;

    /**
     * Finds a filename for sample with the specified gui event.
     * Returns NULL on error or the full path to the sample file.
     * The string returned has to be freed by the caller
     */
    virtual char* findSample(int sound) const = 0;
};

class YAudioInterface {
public:
    virtual ~YAudioInterface() {}
    virtual int init(SoundConf* conf) = 0;

    /**
     * Play the sound for the given event.
     * Return true iff the sound is playable.
     */
    virtual bool play(int sound) = 0;
    virtual void reload() {}
    virtual void drain() {}
};

/******************************************************************************
 * ALSA audio interface
 ******************************************************************************/

#ifdef ENABLE_ALSA

#include <alsa/asoundlib.h>

class YALSAAudio : public YAudioInterface {
public:
    YALSAAudio();
    virtual ~YALSAAudio();

    virtual bool play(int sound);
    virtual int init(SoundConf* conf) {
        this->conf = conf;
        return open();
    }
    virtual void drain() {
        if (playback_handle)
            snd_pcm_drain(playback_handle);
    }

private:
    int open();

    SoundConf* conf;
    snd_pcm_t *playback_handle;
};

YALSAAudio::YALSAAudio() :
    conf(nullptr),
    playback_handle(nullptr)
{
}

YALSAAudio::~YALSAAudio() {
    if (playback_handle) {
        snd_pcm_hw_free(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = nullptr;
    }
}

int YALSAAudio::open() {
    int err;
    if ((err = snd_pcm_open(&playback_handle,
                    conf->alsaDevice(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        warn("cannot open audio device %s (%s)\n",
                 conf->alsaDevice(),
                 snd_strerror(err));
        return ICESOUND_IF_ERROR;
    }
    return ICESOUND_SUCCESS;
}

/**
 * Play a sound sample directly to the digital signal processor.
 */
bool YALSAAudio::play(int sound) {
    csmart samplefile(conf->findSample(sound));
    if (samplefile == nullptr)
        return false;

    if (conf->verbose())
        tlog(_("Playing sample #%d (%s)"), sound, (char *) samplefile);

    int err;
    snd_pcm_hw_params_t *hw_params(nullptr);

    SF_INFO sfinfo = {};
    SNDFILE* sf = sf_open(samplefile, SFM_READ, &sfinfo);
    if (sf == nullptr) {
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
                    (unsigned int *)&sfinfo.samplerate, nullptr)) < 0) {
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
            if ((err = snd_pcm_writei(playback_handle, sbuf, n)) != n) {
                warn("write to audio interface failed (%s) %zd %d\n",
                         snd_strerror(err), sizeof(short), n);
                goto done;
            }
        }
    }

done:
    if (hw_params)
        snd_pcm_hw_params_free(hw_params);
    if (sf)
        sf_close(sf);
    return true;
}

#endif /* ENABLE_ALSA */

/******************************************************************************
 * General (OSS) audio interface
 ******************************************************************************/

#ifdef ENABLE_OSS

class YOSSAudio : public YAudioInterface {
public:
    YOSSAudio(): conf(nullptr), device(-1) {}
    ~YOSSAudio();

    virtual bool play(int sound);
    virtual int init(SoundConf* conf);

private:
    SoundConf* conf;
    int device;
};

/**
 * Play a sound sample directly to the digital signal processor.
 */
bool YOSSAudio::play(int sound) {
    csmart samplefile(conf->findSample(sound));
    if (samplefile == nullptr)
        return false;

    if (conf->verbose())
        tlog(_("Playing sample #%d (%s)"), sound, (char *) samplefile);

    SF_INFO sfinfo = {};
    SNDFILE* sf = sf_open(samplefile, SFM_READ, &sfinfo);
    if (sf == nullptr) {
        warn("%s: %s", (char *) samplefile, sf_strerror(sf));
        return false;
    }

    if (sfinfo.channels < 1 || sfinfo.channels > 2) {
        warn(_("%s: Invalid number of channels"), (char *) samplefile);
        sf_close(sf);
        return false;
    }

    if (ioctl(device, SNDCTL_DSP_CHANNELS, &sfinfo.channels)) {
        fail(_("Could not set OSS channels"));
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_SPEED, &sfinfo.samplerate)) {
        fail(_("Could not set OSS channels"));
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_SYNC, NULL)) {
        fail(_("Could not sync OSS"));
        goto done;
    }

    {
        const int N = 8*1024;
        short sbuf[N] = {};
        const int frames = N / sfinfo.channels;
        for (int count; (count = sf_readf_short(sf, sbuf, frames)) > 0; ) {
            int bytes = count * sfinfo.channels * 2;
            int wrote = write(device, sbuf, bytes);
            if (wrote == -1) {
                fail(_("OSS write failed"));
                goto done;
            }
            if (wrote != bytes) {
                warn(_("OSS incomplete write (%d/%d)"), wrote, bytes);
                break;
            }
        }
    }

    if (ioctl(device, SNDCTL_DSP_POST, NULL)) {
        fail(_("Could not post OSS"));
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_SYNC, NULL)) {
        fail(_("Could not sync OSS"));
        goto done;
    }

done:
    sf_close(sf);
    return true;
}

int YOSSAudio::init(SoundConf* conf) {
    this->conf = conf;
    int result = ICESOUND_IF_ERROR;
    int stereo = 1;
    int format =
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        AFMT_S16_LE
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        AFMT_S16_BE
#else
#error Undefined byte order
#endif
        ;

    if ((device = open(conf->ossDevice(), O_WRONLY)) == -1) {
        fail(_("Could not open OSS device %s"), conf->ossDevice());
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_STEREO, &stereo) == -1) {
        fail(_("Could not set OSS stereo"));
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_RESET, NULL) == -1) {
        fail(_("Could not reset OSS DSP"));
        goto done;
    }

    if (ioctl(device, SNDCTL_DSP_SETFMT, &format) == -1) {
        fail(_("Could not set OSS format"));
        goto done;
    }

    result = ICESOUND_SUCCESS;
done:
    if (device >= 0 && result == ICESOUND_IF_ERROR) {
        close(device);
        device = -1;
    }
    return result;
}

YOSSAudio::~YOSSAudio() {
    if (device >= 0) {
        close(device);
        device = -1;
    }
}

#endif /* ENABLE_OSS */

/******************************************************************************
 * LibAO cross-platform audio output library version 1.2.0 or later.
 ******************************************************************************/

#ifdef ENABLE_AO

#include <ao/ao.h>

class YAOAudio : public YAudioInterface {
public:
    YAOAudio() :
        conf(nullptr),
        driver(-1)
    {
    }
    virtual ~YAOAudio() {
        if (conf)
            ao_shutdown();
    }
    virtual bool play(int sound);
    virtual int init(SoundConf* conf);

private:
    SoundConf* conf;
    int driver;
};

int YAOAudio::init(SoundConf* conf) {
    this->conf = conf;
    ao_initialize();
    driver = ao_default_driver_id();
    if (driver < 0)
        return ICESOUND_IF_ERROR;
    return ICESOUND_SUCCESS;
}

/**
 * Play a cached sound sample using ESounD.
 */
bool YAOAudio::play(int sound) {
    csmart samplefile(conf->findSample(sound));
    if (samplefile == nullptr)
        return false;

    if (conf->verbose())
        tlog(_("Playing sample #%d (%s)"), sound, (char *) samplefile);

    ao_device* device = nullptr;
    ao_sample_format format = {};

    SF_INFO sfinfo = {};
    SNDFILE* sf = sf_open(samplefile, SFM_READ, &sfinfo);
    if (sf == nullptr) {
        warn("%s: %s", (char *) samplefile, sf_strerror(sf));
        goto done;
    }

    format.bits = 16;
    format.rate = sfinfo.samplerate;
    format.channels = sfinfo.channels;
    format.byte_format = AO_FMT_LITTLE;
    format.matrix = nullptr;

    device = ao_open_live(driver, &format, nullptr);
    if (device == nullptr) {
        warn(_("ao_open_live failed with %d"), errno);
        goto done;
    }

    {
        const int N = 8*1024;
        short sbuf[N] = {};
        const int frames = N / format.channels;
        for (int n, k; (n = sf_readf_short(sf, sbuf, frames)) > 0; ) {
            k = ao_play(device, (char *) sbuf, n * 2 * format.channels);
            if (k == 0) {
                warn(_("ao_play failed"));
                goto done;
            }
        }
    }

done:
    if (device)
        ao_close(device);
    if (sf)
        sf_close(sf);
    return true;
}

#endif /* ENABLE_AO */

/******************************************************************************
 * IceSound application
 ******************************************************************************/

class IceSound : public SoundConf {
public:
    IceSound();
    virtual ~IceSound() {}

    void parseArgs(int argc, char** argv);

    virtual bool verbose() const { return verbosity; }
    virtual const char* alsaDevice() const {
        return alsaDeviceFile ? alsaDeviceFile :
            deviceFile ? deviceFile : ALSA_DEFAULT_DEVICE;
    }
    virtual const char* ossDevice() const {
        return ossDeviceFile ? ossDeviceFile :
            deviceFile ? deviceFile : OSS_DEFAULT_DEVICE;
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
    char const* displayName;
    char const* interfaceNames;
    class YAudioInterface* audio;
    upath paths[6];
    Atom _GUI_EVENT;
    Display* display;
    Window root;
    timeval last;
    long snooze;

    const char* name(int sound) const {
        return gui_event_names[sound];
    }
    void initPaths();
    void initSignals();
    int chooseInterface();
    void loopEvents();
    void readEvents();
    void guiEvent();
    int getProperty();
    void playOnce(char* name);
    void nosupport(const char* name);

private:
    static void stop(int sig);
    static void hup(int sig);
};

IceSound::IceSound() :
    verbosity(false),
    sampleDir(nullptr),
    deviceFile(nullptr),
    alsaDeviceFile(nullptr),
    ossDeviceFile(nullptr),
    displayName(nullptr),
    interfaceNames(audio_interfaces),
    audio(nullptr),
    _GUI_EVENT(None),
    display(nullptr),
    root(None),
    last(zerotime()),
    snooze(DEFAULT_SNOOZE_TIME)
{
#ifdef DEBUG
    verbosity = true;
#endif
    initPaths();
    initSignals();
}

void IceSound::parseArgs(int argc, char** argv)
{
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char* value(nullptr);
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
                /*ignore*/;
            }
            else if (GetArgument(value, "z", "snooze", arg, argv + argc)) {
                long t = strtol(value, nullptr, 10);
                if (t > 0) snooze = t;
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
            else if (is_copying_switch(*arg)) {
                print_copying_exit();
            }
            else if (is_switch(*arg, "l", "list-files")) {
                for (int i = 0; i < NUM_GUI_EVENTS; ++i) {
                    if (i != geCloseAll) {
                        csmart samplefile(findSample(i));
                        if (samplefile)
                            printf("%s\n", (char *) samplefile);
                    }
                }
                ::exit(0);
            }
            else if (is_long_switch(*arg, "list-interfaces")) {
                printf("%s\n", audio_interfaces);
                ::exit(0);
            }
            else if (is_long_switch(*arg, "list-sounds")) {
                for (int i = 0; i < NUM_GUI_EVENTS; ++i) {
                    if (i != geCloseAll) {
                        printf("%s.wav\n", name(i));
                    }
                }
                ::exit(0);
            }
            else if (GetArgument(value, "p", "play", arg, argv + argc)) {
                playOnce(value);
                ::exit(0);
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
The currently configured sound interfaces are: %s.\n\
Icesound will choose the first of these which is usable.\n\
\n\
Options:\n\
\n\
 -d, --display=DISPLAY   X11 display used by IceWM (default: $DISPLAY).\n\
\n\
 -s, --sample-dir=DIR    Specifies a directory with sound files.\n\
                         Default is $HOME/.config/icewm/sounds.\n\
\n\
 -i, --interface=LIST    Specifies audio output interfaces. One or more of:\n\
                         %s separated by commas.\n\
\n\
 -D, --device=DEVICE     Backwards compatibility only: the default device. \n\
                         Please prefer one of the -A/-O/-S options.\n\
\n\
 -O, --oss=DEVICE        Specifies the OSS device (default: \"%s\").\n\
\n\
 -A, --alsa=DEVICE       Specifies the ALSA device (default: \"%s\").\n\
\n\
 -z, --snooze=millisecs  Specifies the snooze interval between sound events\n\
                         in milliseconds. Default is 500 milliseconds.\n\
\n\
 -p, --play=sound        Plays the given sound (name or number) and exits.\n\
\n\
 -l, --list-files        Lists the available sound file paths and exits.\n\
\n\
     --list-sounds       Lists the supported sound filenames and exits.\n\
\n\
     --list-interfaces   Lists the supported audio interfaces and exits.\n\
\n\
 -v, --verbose           Be verbose and print out each sound event.\n\
\n\
 -V, --version           Prints version information and exits.\n\
\n\
 -h, --help              Prints this help screen and exits.\n\
\n\
Return values:\n\
\n\
  0    Success.\n\
  1    General error.\n\
  2    Command line error.\n\
  3    Subsystems error (ie cannot connect to server).\n\n"),
           ApplicationName, audio_interfaces, audio_interfaces,
           ALSA_DEFAULT_DEVICE, OSS_DEFAULT_DEVICE);

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

void IceSound::initSignals() {
    signal(SIGINT, stop); // === ensure clean exit ===
    signal(SIGTERM, stop);
    signal(SIGPIPE, stop);
    signal(SIGHUP, hup);
}

/**
 * Finds a filename for sample with the specified gui event.
 * Returns NULL on error or the full path to the sample file.
 * The string returned has to be freed by the caller
 */
char* IceSound::findSample(int sound) const {
    if (inrange(sound, 0, NUM_GUI_EVENTS - 1)) {
        upath base(mstring(name(sound), ".wav"));
        for (int k = 0; paths[k] != null; ++k) {
            upath file(paths[k] + base);
            if (file.isReadable()) {
                return newstr(file.string());
            }
        }
        if (verbose())
            tlog(_("No audio for %s"), name(sound));
    }
    return nullptr;
}

/**
 * The hearth of icesound
 */
int IceSound::run() {
    MSG(("Compiled with DEBUG flag. Debugging messages will be printed."));

    int rc = chooseInterface();
    if (rc) return rc;

    if (nullptr == (display = XOpenDisplay(displayName))) { // === connect to X11 ===
        warn(_("Can't open display: %s. X must be running and $DISPLAY set."),
             XDisplayName(displayName));
        rc = ICESOUND_IF_ERROR;
    }
    else {
        root = RootWindow(display, DefaultScreen(display));
        _GUI_EVENT = XInternAtom(display, XA_GUI_EVENT_NAME, False);
        XSelectInput(display, root, PropertyChangeMask);
        loopEvents();
        XCloseDisplay(display);
        display = nullptr;
    }
    delete audio;
    audio = nullptr;
    return rc;
}

void IceSound::loopEvents() {
    int fd = ConnectionNumber(display);
    fd_set rfds;
    FD_ZERO(&rfds);
    for (readEvents(); soundAsync.running; readEvents()) {
        FD_SET(fd, &rfds);
        select(fd + 1, SELECT_TYPE_ARG234 &rfds, nullptr, nullptr, nullptr);
    }
}

void IceSound::readEvents() {
    while (XPending(display) && soundAsync.running) {
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
    unsigned char *propdata(nullptr);
    int gev(-1);

    if (XGetWindowProperty(display, root, _GUI_EVENT,
                           0, 1, False, _GUI_EVENT,
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
        tlog(_("Received GUI event %s"), name(gev));

    /* Received restart event? */
    if (geRestart == gev) { // --- restart event ---
        MSG(("Restart event received"));
        soundAsync.reload = true;
    }

    timeval now = monotime();
    if (last + millitime(snooze) < now || gev == geStartup) {
        if (audio->play(gev))
            last = now;
    }
    else if (verbose())
        tlog(_("Too quick; ignoring %s."), name(gev));
}

void IceSound::nosupport(const char* name) {
    warn(_("Support for the %s interface not compiled."), name);
}

int IceSound::chooseInterface() {
    int rc(ICESOUND_IF_ERROR);
    mstring name, list(interfaceNames);
    while (audio == nullptr && list.splitall(',', &name, &list)) {
        if (name.isEmpty())
            continue;
        name = name.upper();
        const char* val = name;
        if (name == "OSS") {
#ifdef ENABLE_OSS
            audio = new YOSSAudio();
#else
            nosupport(val);
#endif
        }
        else if (name == "ALSA") {
#ifdef ENABLE_ALSA
            audio = new YALSAAudio();
#else
            nosupport(val);
#endif
        }
        else if (name == "AO") {
#ifdef ENABLE_AO
            audio = new YAOAudio();
#else
            nosupport(val);
#endif
        }
        else {
            warn(_("Unsupported interface: %s."), val);
        }
        if (audio) {
            rc = audio->init(this);
            if (rc) {
                delete audio;
                audio = nullptr;
            }
            else if (verbose())
                tlog(_("Using %s audio."), val);
        }
    }
    if (audio == nullptr) {
        warn(_("Failed to connect to audio interfaces %s."), interfaceNames);
    }

    return rc;
}

/**
 * Signal handler.
 */
void IceSound::stop(int sig) {
    signal(sig, SIG_DFL);

    msg(_("Received signal %s: Terminating..."), strsignal(sig));
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

void IceSound::playOnce(char* value) {
    char* end = value;
    int sound = (int) strtol(value, &end, 10);
    if (end <= value || *end) {
        sound = -1;
        for (int i = 0; i < NUM_GUI_EVENTS && sound < 0; ++i)
            if (0 == strncmp(value, name(i), strlen(name(i))))
                sound = i;
    }
    if (inrange(sound, 0, NUM_GUI_EVENTS - 1)) {
        if (chooseInterface() == ICESOUND_SUCCESS) {
            if (audio->play(sound)) {
                audio->drain();
            }
            delete audio; audio = nullptr;
        }
    }
    else {
        warn(_("Unrecognized argument: %s\n"), value);
    }
}

int main(int argc, char *argv[]) {

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    ApplicationName = my_basename(argv[0]);

    IceSound icesound;
    icesound.parseArgs(argc, argv);
    return icesound.run();
}


// vim: set sw=4 ts=4 et:
