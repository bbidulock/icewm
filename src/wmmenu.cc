/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmprog.h"
#include "yconfig.h"
#include "wmapp.h"
#include "sysdep.h"
#include "ascii.h"
#include "argument.h"
#include "intl.h"

static char* getWord(char* word, size_t wordsize, char* start) {
    char *p = start;
    while (ASCII::isAlnum(*p))
        ++p;

    size_t length = p - start;
    strlcpy(word, start, min(1 + length, wordsize));

    while (*p == ' ')
        ++p;
    return p;
}

static char *getCommandArgs(char *source, Argument *command,
                            YStringArray &args)
{
    char *p = source;
    p = YConfig::getArgument(command, p);
    if (p == 0) {
        msg(_("Missing command argument"));
        return p;
    }
    args.append(*command);

    Argument argx;
    while (*p) {

        //push to the next word or line end to get the arg
        while (ASCII::isSpaceOrTab(*p))
            p++;
        //stop on EOL
        if (*p == '\n')
            break;

        // parse the argument into argx and set the new position
        p = YConfig::getArgument(&argx, p);
        if (p == 0) {
            msg(_("Bad argument %d to command '%s'"),
                    args.getCount() + 1, command->cstr());
            return p;
        }

        args.append(argx);
        MSG(("ARG: %s\n", argx.cstr()));
    }
    args.append(0);

    return p;
}

static ustring guessIconNameFromExe(const char* exe)
{
    upath fullname(exe);
    char buf[1024];
    for (int i=7; i; --i)
    {
        fullname = findPath(getenv("PATH"), X_OK, fullname);
        if (fullname == null)
            return "-";
        ssize_t linkLen = readlink(fullname.string().c_str(), buf, ACOUNT(buf));
        if (linkLen < 0)
            break;
        fullname = upath(buf, linkLen);
    }
    // crop to the generic name
    ustring s(fullname);
    int spos = s.lastIndexOf('/');
    if (spos >= 0)
        s = s.remove(0, spos + 1);
    // scripts have a suffix sometimes which is not part of the icon name
    spos = s.indexOf('.');
    if (spos >= 0)
        s = s.substring(0, spos);
    return s;
}

char* MenuLoader::parseKey(char *word, char *p)
{
    bool runonce = !strcmp(word, "runonce");
    bool switchkey = !strcmp(word, "switchkey");

    Argument key;

    p = YConfig::getArgument(&key, p);
    if (p == 0) return p;

    Argument wmclass;

    if (runonce) {
        p = YConfig::getArgument(&wmclass, p);
        if (p == 0) return p;
    }

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == 0) {
        msg(_("Error at keyword '%s' for %s"), word, key.cstr());
        return p;
    }

    DProgram *prog = DProgram::newProgram(
        app,
        smActionListener,
        key,
        null,
        false,
        runonce ? wmclass.cstr() : NULL,
        command.cstr(),
        args);

    if (prog) new KProgram(key, prog, switchkey);

    return p;
}

char* MenuLoader::parseProgram(char *word, char *p, ObjectContainer *container)
{
    bool runonce = !strcmp(word, "runonce");
    bool restart = !strcmp(word, "restart");

    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == 0) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == 0) return p;

    Argument wmclass;

    if (runonce) {
        p = YConfig::getArgument(&wmclass, p);
        if (p == 0) return p;
    }

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == 0) {
        msg(_("Error at %s '%s'"), word, name.cstr());
        return p;
    }

    ref<YIcon> icon;

    if (icons[0] == '!')
    {
        ustring iconName = guessIconNameFromExe(command);
        if (iconName.charAt(0) != '-')
            icon = YIcon::getIcon(cstring(iconName));

    }
    else if (icons[0] != '-')
        icon = YIcon::getIcon(icons);

    DProgram * prog = DProgram::newProgram(
        app,
        smActionListener,
        name,
        icon,
        restart,
        runonce ? wmclass.cstr() : NULL,
        command.cstr(),
        args);

    if (prog) container->addObject(prog);

    return p;
}

char* MenuLoader::parseAMenu(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == 0) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == 0) return p;

    while (ASCII::isWhiteSpace(*p))
        ++p;

    char word[32];
    p = getWord(word, sizeof(word), p);
    if (*p != '{') return 0;
    p++;

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);

    ObjectMenu *sub = new ObjectMenu(wmActionListener);

    if (sub) {
        p = parseMenus(p, sub);

        if (sub->itemCount() == 0)
            delete sub;
        else
            container->addContainer(name.cstr(), icon, sub);

    } else {
        msg(_("Unexepected menu keyword: '%s'"), name.cstr());
        return p;
    }
    return p;
}

char* MenuLoader::parseMenuFile(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == 0) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == 0) return p;

    Argument menufile;

    p = YConfig::getArgument(&menufile, p);
    if (p == 0) return p;

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);

    ObjectMenu *filemenu = new MenuFileMenu(
            app, smActionListener, wmActionListener,
            menufile.cstr(), 0);

    if (menufile)
        container->addContainer(name.cstr(), icon, filemenu);

    return p;
}

char* MenuLoader::parseMenuProg(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == 0) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == 0) return p;

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == 0) {
        msg(_("Error at menuprog '%s'"), name.cstr());
        return p;
    }

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);
    MSG(("menuprog %s %s", name.cstr(), command.cstr()));

    upath fullPath = findPath(getenv("PATH"), X_OK, command.cstr());
    if (fullPath != null) {
        ObjectMenu *progmenu = new MenuProgMenu(
                app, smActionListener, wmActionListener,
                name.cstr(), command.cstr(), args);
        if (progmenu)
            container->addContainer(name.cstr(), icon, progmenu);
    }

    return p;
}

char* MenuLoader::parseMenuProgReload(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == 0) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == 0) return p;

    Argument timeoutStr;

    p = YConfig::getArgument(&timeoutStr, p);
    if (p == 0) return p;
    long timeout = atol(timeoutStr);

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == 0) {
        msg(_("Error at menuprogreload: '%s'"), name.cstr());
        return p;
    }

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);
    MSG(("menuprogreload %s %s", name.cstr(), command.cstr()));

    upath fullPath = findPath(getenv("PATH"), X_OK, command.cstr());
    if (fullPath != null) {
        ObjectMenu *progmenu = new MenuProgMenu(
                app, smActionListener, wmActionListener,
                name.cstr(), command.cstr(), args, timeout);
        if (progmenu)
            container->addContainer(name.cstr(), icon, progmenu);
    }

    return p;
}

char* MenuLoader::parseIncludeStatement(char *p, ObjectContainer *container)
{
    Argument filename;

    p = YConfig::getArgument(&filename, p);
    if (p == 0) {
        warn(_("Missing filename argument to include statement"));
        return p;
    }

    upath path(app->findConfigFile(filename.cstr()));
    if (path != null)
        loadMenus(path, container);

    return p;
}

char* MenuLoader::parseIncludeProgStatement(char *p, ObjectContainer *container)
{
    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == 0) {
        msg(_("Error at includeprog '%s'"), command.cstr());
        return p;
    }

    progMenus(command.cstr(), args.getCArray(), container);

    return p;
}

char* MenuLoader::parseWord(char *word, char *p, ObjectContainer *container)
{
    if (container) {
        if (!strcmp(word, "separator")) {
            container->addSeparator();
        }
        else if (!(strcmp(word, "prog") &&
                   strcmp(word, "restart") &&
                   strcmp(word, "runonce")))
        {
            p = parseProgram(word, p, container);
        }
        else if (!strcmp(word, "menu")) {
            p = parseAMenu(p, container);
        }
        else if (!strcmp(word, "menufile")) {
            p = parseMenuFile(p, container);
        }
        else if (!strcmp(word, "menuprog")) {
            p = parseMenuProg(p, container);
        }
        else if (!strcmp(word, "menuprogreload")) {
            p = parseMenuProgReload(p, container);
        }
        else if (!strcmp(word, "include")) {
            p = parseIncludeStatement(p, container);
        }
        else if (!strcmp(word, "includeprog")) {
            p = parseIncludeProgStatement(p, container);
        }
        else if (*p == '}') {
            return p;
        }
        else {
            msg(_("Unknown keyword '%s'"), word);
            return 0;
        }
    }
    else if (!strcmp(word, "key")
          || !strcmp(word, "runonce")
          || !strcmp(word, "switchkey"))
    {
        p = parseKey(word, p);
    }
    else {
        msg(_("Unknown keyword for a non-container: '%s'.\n"
              "Expected either 'key' or 'runonce' here.\n"), word);
        return 0;
    }
    return p;
}

char* MenuLoader::parseMenus(char *data, ObjectContainer *container)
{
    for (char* p = data; p && *p; ) {
        if (ASCII::isWhiteSpace(*p)) {
            ++p;
        }
        else if (*p == '#') {
            p = strchr(p, '\n');
        }
        else if (*p == '}') {
            return ++p;
        }
        else {
            char word[32];
            p = getWord(word, sizeof(word), p);
            p = parseWord(word, p, container);
        }
    }
    return 0;
}

void MenuLoader::loadMenus(upath menufile, ObjectContainer *container)
{
    if (menufile.isEmpty())
        return;

    MSG(("menufile: %s", menufile.string().c_str()));
    char *buf = load_text_file(menufile.string());
    if (buf) {
        parseMenus(buf, container);
        delete[] buf;
    }
}

void MenuLoader::progMenus(
    const char *command,
    char *const argv[],
    ObjectContainer *container)
{
    fileptr fpt(tmpfile());
    if (fpt == 0) {
        fail("tmpfile");
        return;
    }

    int tfd = fileno(fpt);
    int status = 0;
    pid_t child_pid = fork();

    if (child_pid == -1) {
        fail("Forking '%s' failed", command);
    }
    else if (child_pid == 0) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull > 0) {
            dup2(devnull, 0);
            close(devnull);
        }
        if (dup2(tfd, 1) == 1) {
            if (tfd > 2) close(tfd);
            execvp(command, argv);
        }
        fail("Exec '%s' failed", command);
        _exit(99);
    }
    else if (waitpid(child_pid, &status, 0) == 0 && status != 0) {
        warn("'%s' exited with code %d.", command, status);
    }
    else if (lseek(tfd, (off_t) 0L, SEEK_SET) == (off_t) -1) {
        fail("lseek failed");
    }
    else {
        char *buf = load_fd(tfd);
        fpt.close();
        if (buf && *buf) {
            parseMenus(buf, container);
        }
        else {
            warn(_("'%s' produces no output"), command);
        }
        delete[] buf;
    }
}

// vim: set sw=4 ts=4 et:
