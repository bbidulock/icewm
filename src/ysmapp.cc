#include "config.h"

#ifdef CONFIG_SESSION

#include "ysmapp.h"
#include "sysdep.h"
#include "base.h"
#include "yconfig.h"

#include "intl.h"

#include <X11/SM/SMlib.h>

YSMApplication *smapp = 0;
int IceSMfd = -1;
IceConn IceSMconn = NULL;
SmcConn SMconn = NULL;
char *oldSessionId = NULL;
char *newSessionId = NULL;
char *sessionProg;

char *getsesfile() {
    static char filename[PATH_MAX] = "";

    if (*filename == '\0') {
        strcpy(filename, YApplication::getPrivConfDir());
        mkdir(filename, 0755);

        strcat(filename, "/.session-");
        strcat(filename, newSessionId);

        MSG(("Storing session in %s", filename));
    }

    return filename;
}

static void iceWatchFD(IceConn conn,
                       IcePointer /*client_data*/,
                       Bool opening,
                       IcePointer */*watch_data*/)
{
    if (opening) {
        if (IceSMfd != -1) { // shouldn't happen
            warn("TOO MANY ICE CONNECTIONS -- not supported");
        } else {
            IceSMfd = IceConnectionNumber(conn);
            fcntl(IceSMfd, F_SETFD, FD_CLOEXEC);
        }
    } else {
        if (IceConnectionNumber(conn) == IceSMfd)
            IceSMfd = -1;
    }
}

void saveYourselfPhase2Proc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    smapp->smSaveYourselfPhase2();
}

void saveYourselfProc(SmcConn /*conn*/,
                      SmPointer /*client_data*/,
                      int /*save_type*/,
                      Bool shutdown,
                      int /*interact_style*/,
                      Bool fast)
{
    smapp->smSaveYourself(shutdown ? true : false, fast ? true : false);
}

void shutdownCancelledProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    smapp->smShutdownCancelled();
}

void saveCompleteProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    smapp->smSaveComplete();
}

void dieProc(SmcConn /*conn*/, SmPointer /*client_data*/) {
    smapp->smDie();
}

static void setSMProperties() {
    SmPropValue programVal = { 0, NULL };
    SmPropValue userIDVal = { 0, NULL };
    SmPropValue restartVal[3] = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
    SmPropValue discardVal[4] = { { 0, NULL }, { 0, NULL }, { 0, NULL } };

    // broken headers in SMlib?
    SmProp programProp = { (char *)SmProgram, (char *)SmLISTofARRAY8, 1, &programVal };
    SmProp userIDProp = { (char *)SmUserID, (char *)SmARRAY8, 1, &userIDVal };
    SmProp restartProp = { (char *)SmRestartCommand, (char *)SmLISTofARRAY8, 3, (SmPropValue *)&restartVal };
    SmProp cloneProp = { (char *)SmCloneCommand, (char *)SmLISTofARRAY8, 1, (SmPropValue *)&restartVal };
    SmProp discardProp = { (char *)SmDiscardCommand, (char *)SmLISTofARRAY8, 3, (SmPropValue *)&discardVal };
    SmProp *props[] = {
        &programProp,
        &userIDProp,
        &restartProp,
        &cloneProp,
        &discardProp
    };

    char *user = getenv("USER");
    if (!user) // not a user?
        user = getenv("LOGNAME");
    if (!user) {
        msg(_("$USER or $LOGNAME not set?"));
        return ;
    }
    const char *clientId = "--client-id";

    programVal.length = strlen(sessionProg);
    programVal.value = sessionProg;
    userIDVal.length = strlen(user);
    userIDVal.value = (SmPointer)user;

    restartVal[0].length = strlen(sessionProg);
    restartVal[0].value = sessionProg;
    restartVal[1].length = strlen(clientId);
    restartVal[1].value = (char *)clientId;
    restartVal[2].length = strlen(newSessionId);
    restartVal[2].value = newSessionId;

    const char *rmprog = "/bin/rm";
    const char *rmarg = "-f";
    char *sidfile = getsesfile();

    discardVal[0].length = strlen(rmprog);
    discardVal[0].value = (char *)rmprog;
    discardVal[1].length = strlen(rmarg);
    discardVal[1].value = (char *)rmarg;
    discardVal[2].length = strlen(sidfile);
    discardVal[2].value = sidfile;

    SmcSetProperties(SMconn,
                     sizeof(props)/sizeof(props[0]),
                     (SmProp **)&props);
}

static void initSM() {
    if (getenv("SESSION_MANAGER") == 0)
        return;
    if (IceAddConnectionWatch(&iceWatchFD, NULL) == 0) {
        warn("Session Manager: IceAddConnectionWatch failed.");
        return ;
    }

    char error_str[256];
    SmcCallbacks smcall;

    memset(&smcall, 0, sizeof(smcall));
    smcall.save_yourself.callback = &saveYourselfProc;
    smcall.save_yourself.client_data = NULL;
    smcall.die.callback = &dieProc;
    smcall.die.client_data = NULL;
    smcall.save_complete.callback = &saveCompleteProc;
    smcall.save_complete.client_data = NULL;
    smcall.shutdown_cancelled.callback = &shutdownCancelledProc;
    smcall.shutdown_cancelled.client_data = NULL;

    if ((SMconn = SmcOpenConnection(NULL, /* network ids */
                                    NULL, /* context */
                                    1, 0, /* protocol major, minor */
                                    SmcSaveYourselfProcMask |
                                    SmcSaveCompleteProcMask |
                                    SmcShutdownCancelledProcMask |
                                    SmcDieProcMask,
                                    &smcall,
                                    oldSessionId, &newSessionId,
                                    sizeof(error_str), error_str)) == NULL)
    {
        warn("Session Manager: Init error: %s", error_str);
        return ;
    }
    IceSMconn = SmcGetIceConnection(SMconn);

    setSMProperties();
}

void YSMApplication::smSaveYourself(bool /*shutdown*/, bool /*fast*/) {
    SmcRequestSaveYourselfPhase2(SMconn, &saveYourselfPhase2Proc, NULL);
}

void YSMApplication::smSaveYourselfPhase2() {
    SmcSaveYourselfDone(SMconn, True);
}

void YSMApplication::smSaveDone() {
    SmcSaveYourselfDone(SMconn, True);
}

void YSMApplication::smSaveComplete() {
}

void YSMApplication::smShutdownCancelled() {
    SmcSaveYourselfDone(SMconn, False);
}

void YSMApplication::smCancelShutdown() {
    SmcSaveYourselfDone(SMconn, False); /// !!! broken
}

void YSMApplication::smDie() {
    app->exit(0);
}

bool YSMApplication::haveSessionManager() {
    if (SMconn != NULL)
        return true;
    return false;
}

void YSMApplication::smRequestShutdown() {
    // !!! doesn't seem to work with xsm
    SmcRequestSaveYourself(SMconn,
                           SmSaveLocal, //!!!
                           True,
                           SmInteractStyleAny,
                           False,
                           True);
}

YSMApplication::YSMApplication(int *argc, char ***argv, const char *displayName):
YXApplication(argc, argv, displayName)
{
    smapp = this;
    for (char ** arg = *argv + 1; arg < *argv + *argc; ++arg) {
        if (**arg == '-') {
            char *value;

            if ((value = GET_LONG_ARGUMENT("client-id")) != NULL)
                oldSessionId = value;
        }
    }

    sessionProg = (*argv)[0]; //ICEWMEXE;
    initSM();
    psm.registerPoll(this, IceSMfd);
}

YSMApplication::~YSMApplication() {
    if (SMconn != 0) {
        SmcCloseConnection(SMconn, 0, NULL);
        SMconn = NULL;
        IceSMconn = NULL;
        IceSMfd = -1;
        unregisterPoll(&psm);
    }
}

void YSMPoll::notifyRead() {
    Bool rep;
    if (IceProcessMessages(IceSMconn, NULL, &rep)
        == IceProcessMessagesIOError)
    {
        SmcCloseConnection(SMconn, 0, NULL);
        IceSMconn = NULL;
        IceSMfd = -1;
        unregisterPoll();
    }
}

void YSMPoll::notifyWrite() {
}

bool YSMPoll::forRead() {
    return true;
}

bool YSMPoll::forWrite() {
    return false;
}

#endif /* CONFIG_SESSION */
