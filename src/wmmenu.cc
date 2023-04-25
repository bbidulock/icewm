/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmprog.h"
#include "yconfig.h"
#include "ypointer.h"
#include "wmapp.h"
#include "sysdep.h"
#include "ascii.h"
#include "argument.h"
#include "intl.h"

#define TIMEOUT_MS 2000

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
    if (p == nullptr) {
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
        if (p == nullptr) {
            msg(_("Bad argument %d to command '%s'"),
                    args.getCount() + 1, command->cstr());
            return p;
        }

        args.append(argx);
        MSG(("ARG: %s\n", argx.cstr()));
    }
    args.append(nullptr);

    return p;
}

static mstring guessIconNameFromExe(const char* exe)
{
    csmart path(path_lookup(exe));
    for (int i = 7; i && path; --i) {
        char buf[PATH_MAX];
        ssize_t linkLen = readlink(path, buf, PATH_MAX);
        if (linkLen < 0)
            break;
        path = newstr(buf, linkLen);
    }
    if (path) {
        char* base = const_cast<char*>(my_basename(path));
        // scripts may have a suffix which is not part of the icon name
        char* dot = strchr(base, '.');
        if (dot) *dot = '\0';
        return base;
    }
    return "-";
}

char* MenuLoader::parseKey(char *word, char *p)
{
    bool runonce = !strcmp(word, "runonce");
    bool switchkey = !strcmp(word, "switchkey");

    Argument key;

    p = YConfig::getArgument(&key, p);
    if (p == nullptr) return p;

    Argument wmclass;

    if (runonce) {
        p = YConfig::getArgument(&wmclass, p);
        if (p == nullptr) return p;
    }

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == nullptr) {
        msg(_("Error at keyword '%s' for %s"), word, key.cstr());
        return p;
    }

    DProgram *prog = DProgram::newProgram(
        app,
        smActionListener,
        key,
        null,
        false,
        runonce ? wmclass.cstr() : nullptr,
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
    if (p == nullptr) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == nullptr) return p;

    Argument wmclass;

    if (runonce) {
        p = YConfig::getArgument(&wmclass, p);
        if (p == nullptr) return p;
    }

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == nullptr) {
        msg(_("Error at %s '%s'"), word, name.cstr());
        return p;
    }

    ref<YIcon> icon;

    if (icons[0] == '!')
    {
        mstring iconName = guessIconNameFromExe(command);
        if (iconName.charAt(0) != '-')
            icon = YIcon::getIcon(iconName);

    }
    else if (icons[0] != '-')
        icon = YIcon::getIcon(icons);

    DProgram * prog = DProgram::newProgram(
        app,
        smActionListener,
        name,
        icon,
        restart,
        runonce ? wmclass.cstr() : nullptr,
        command.cstr(),
        args);

    if (prog) container->addObject(prog);

    return p;
}

char* MenuLoader::parseAMenu(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == nullptr) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == nullptr) return p;

    while (ASCII::isWhiteSpace(*p))
        ++p;

    char word[32];
    p = getWord(word, sizeof(word), p);
    if (*p != '{') return nullptr;
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
    if (p == nullptr) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == nullptr) return p;

    Argument menufile;

    p = YConfig::getArgument(&menufile, p);
    if (p == nullptr) return p;

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);

    ObjectMenu *filemenu = new MenuFileMenu(
            app, smActionListener, wmActionListener,
            menufile.cstr(), nullptr);

    if (menufile)
        container->addContainer(name.cstr(), icon, filemenu);

    return p;
}

char* MenuLoader::parseMenuProg(char *p, ObjectContainer *container)
{
    Argument name;

    p = YConfig::getArgument(&name, p);
    if (p == nullptr) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == nullptr) return p;

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == nullptr) {
        msg(_("Error at menuprog '%s'"), name.cstr());
        return p;
    }

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);
    MSG(("menuprog %s %s", name.cstr(), command.cstr()));

    csmart path(path_lookup(command.cstr()));
    if (path) {
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
    if (p == nullptr) return p;

    Argument icons;

    p = YConfig::getArgument(&icons, p);
    if (p == nullptr) return p;

    Argument timeoutStr;

    p = YConfig::getArgument(&timeoutStr, p);
    if (p == nullptr) return p;
    long timeout = atol(timeoutStr);

    Argument command;
    YStringArray args;

    p = getCommandArgs(p, &command, args);
    if (p == nullptr) {
        msg(_("Error at menuprogreload: '%s'"), name.cstr());
        return p;
    }

    ref<YIcon> icon;
    if (icons[0] != '-')
        icon = YIcon::getIcon(icons);
    MSG(("menuprogreload %s %s", name.cstr(), command.cstr()));

    csmart path(path_lookup(command.cstr()));
    if (path) {
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
    if (p == nullptr) {
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
    if (p == nullptr) {
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
            return nullptr;
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
        return nullptr;
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
    return nullptr;
}

void MenuLoader::loadMenus(upath menufile, ObjectContainer *container)
{
    if (menufile.isEmpty())
        return;

    MSG(("menufile: %s", menufile.string()));
    YTraceConfig trace(menufile.string());
    auto buf = menufile.loadText();
    if (buf) parseMenus(buf, container);
}

void MenuLoader::progMenus(
    const char *command,
    char *const argv[],
    ObjectContainer *container)
{
    int fds[2];
    if (pipe(fds) == -1) {
        fail("pipe");
        return;
    }

    int pid = fork();
    if (pid == -1) {
        fail(_("Failed to execute %s"), command);
    }
    else if (pid == 0) {
        close(fds[0]);
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull > 0) {
            dup2(devnull, 0);
            close(devnull);
        }
        if (fds[1] > 1 && (dup2(fds[1], 1) != 1 || close(fds[1]))) {
            fail("dup2!=1");
        }
        else {
            char* path = path_lookup(command);
            if (path) {
                execv(path, argv);
            }
            fail(_("Failed to execute %s"), path ? path : command);
        }
        _exit(99);
    }
    else {
        close(fds[1]);
        bool expired = false;
        filereader rdr(fds[0]);
        auto buf = rdr.read_pipe(TIMEOUT_MS, &expired);
        if (expired) {
            warn(_("'%s' timed out!"), command);
            kill(pid, SIGKILL);
        }
        int status = app->waitProgram(pid);
        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            tlog(_("%s exited with status %d."), command, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            tlog(_("%s was killed by signal %d."), command, WTERMSIG(status));
        }
        else if (nonempty(buf)) {
            parseMenus(buf, container);
        }
        else if (expired == false) {
            warn(_("'%s' produces no output"), command);
        }
    }
}

// vim: set sw=4 ts=4 et:
