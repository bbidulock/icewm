#include "config.h"
#include "yxapp.h"
#include "yinputline.h"
#include "ylabel.h"
#include "ybutton.h"
#include "prefs.h"
#include "yicon.h"
#include <X11/Xatom.h>
#include "ylocale.h"
#include "intl.h"

const char *ApplicationName = "icerun";

class InputRun : public YInputLine {
public:
    InputRun(YWindow *parent, YInputListener *listener) :
        YInputLine(parent, listener)
    {
    }

    virtual void handleClose() {
        xapp->exit(0);
    }

};

class IceRun : public YXApplication, public YInputListener {
    YInputLine *input;
    mstring cmdPrefix;
    ref<YPixmap> large;

public:
    IceRun(int* argc, char*** argv);
    ~IceRun() { delete input; }

    virtual void inputReturn(YInputLine* input, bool control) {
        mstring command(input->getText());
        if (command != cmdPrefix) {
            msg("%s", command.c_str());
            runCommand(command);
        }
        this->exit(0);
    }

    virtual void inputEscape(YInputLine* input) {
        this->exit(0);
    }

    virtual void inputLostFocus(YInputLine* input) {
    }

};

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    IceRun app(&argc, &argv);
    return app.mainLoop();
}

IceRun::IceRun(int* argc, char*** argv) :
    YXApplication(argc, argv),
    input(new InputRun(nullptr, this))
{
    bool redirect = false;
    for (char** arg = *argv + 1; arg < *argv + *argc; ++arg) {
        if (**arg == '-') {
            if (is_short_switch(*arg, "r"))
                redirect = true;
        }
    }

    if (redirect)
        input->addStyle(YWindow::wsOverrideRedirect);

    int textWidth = input->getFont()->textWidth("M");
    int inputWidth = max(300, textWidth * 80);
    int textHeight = input->getFont()->height();
    int inputHeight = max(20, textHeight + 5);
    int xpos = (xapp->displayWidth() - inputWidth) / 2;
    int ypos = (xapp->displayHeight() - inputHeight) / 2;

    input->setGeometry(YRect(xpos, ypos, inputWidth, inputHeight));
    if (nonempty(terminalCommand)) {
        cmdPrefix = mstring(terminalCommand, " -e ");
    }
    input->setText(cmdPrefix, false);
    input->unselectAll();

    ref<YIcon> file = YIcon::getIcon("icewm");
    if (file != null) {
        unsigned depth = xapp->depth();
        large = YPixmap::createFromImage(file->large(), depth);
    }

    static char wm_clas[] = "IceWM";
    static char wm_name[] = "icerun";
    XClassHint class_hint = { wm_name, wm_clas };
    XSizeHints size_hints = { PPosition | PSize, xpos, ypos,
                              inputWidth, inputHeight, };
    XWMHints wmhints = {
        InputHint | StateHint,
        True,
        NormalState,
        large != null ? large->pixmap() : None, None, 0, 0,
        large != null ? large->mask() : None,
        None
    };
    if (wmhints.icon_pixmap)
        wmhints.flags |= IconPixmapHint;
    if (wmhints.icon_mask)
        wmhints.flags |= IconMaskHint;
    Xutf8SetWMProperties(xapp->display(), input->handle(),
                         ApplicationName, "icewm", *argv, *argc,
                         &size_hints, &wmhints, &class_hint);

    input->setNetPid();
    input->show();
    if (redirect)
        XSetInputFocus(xapp->display(), input->handle(),
                       RevertToNone, CurrentTime);
}

// vim: set sw=4 ts=4 et:
