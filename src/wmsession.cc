/*
 * IceWM
 *
 * Copyright (C) 1999-2002 Marko Macek
 *
 * Session management support
 */

#include "config.h"

#ifdef CONFIG_SESSION

#include "yfull.h"
#include "wmframe.h"
#include "wmsession.h"
#include "base.h"
#include "wmapp.h"

#include "mstring.h"
#include <stdio.h>
#include <string.h>

#include "intl.h"

SMWindowKey::SMWindowKey(YFrameWindow */*f*/):
    clientId(null), windowRole(null), windowClass(null), windowInstance(null)
{
}

SMWindowKey::SMWindowKey(ustring id, ustring role):
    clientId(id), windowRole(role), windowClass(null), windowInstance(null)
{
}

SMWindowKey::SMWindowKey(ustring id, ustring klass, ustring instance):
    clientId(id), windowRole(null), windowClass(klass), windowInstance(instance)
{
}

SMWindowKey::~SMWindowKey() {
}

SMWindowInfo::SMWindowInfo(YFrameWindow *f): key(f) {
}

SMWindowInfo::SMWindowInfo(ustring id, ustring role,
                           int ax, int ay, int w, int h,
                           unsigned long astate, int alayer, int aworkspace): key(id, role)
{
    x = ax;
    y = ay;
    width = w;
    height = h;
    state = astate;
    layer = alayer;
    workspace = aworkspace;
}

SMWindowInfo::SMWindowInfo(ustring id, ustring klass, ustring instance,
                           int ax, int ay, int w, int h,
                           unsigned long astate, int alayer, int aworkspace): key(id, klass, instance)
{
    x = ax;
    y = ay;
    width = w;
    height = h;
    state = astate;
    layer = alayer;
    workspace = aworkspace;
}

SMWindowInfo::~SMWindowInfo() {
}

void SMWindows::addWindowInfo(SMWindowInfo *info) {
    fWindows.append(info);
}

void SMWindows::setWindowInfo(YFrameWindow */*f*/) {
}

bool SMWindows::getWindowInfo(YFrameWindow */*f*/, SMWindowInfo */*info*/) {
    return false;
}

bool SMWindows::findWindowInfo(YFrameWindow *f) {
    f->client()->getClientLeader();
    Window leader = f->client()->clientLeader();
    if (leader == None) return false;

    ustring cid = f->client()->getClientId(leader);
    if (cid == null) return false;

    for (int i = 0; i < fWindows.getCount(); ++i) {
        const SMWindowInfo *window = fWindows.getItem(i);

        if (cid.equals(window->key.clientId)) {
            if (window->key.windowClass != null &&
                window->key.windowInstance != null)
            {
                ustring klass = null;
                ustring instance = null;
                XClassHint *ch = f->client()->classHint();

                if (ch) {
                    klass = ch->res_class;
                    instance = ch->res_name;
                }

                if (klass.equals(window->key.windowClass) &&
                    instance.equals(window->key.windowInstance))
                {
                    MSG(("got c %s %s %s %d:%d:%d:%d %d %ld %d",
                         cstring(cid).c_str(), cstring(klass).c_str(), cstring(instance).c_str(),
                         window->x, window->y, window->width, window->height,
                         window->workspace, window->state, window->layer));
                    f->configureClient(window->x, window->y,
                                       window->width, window->height);
                    f->setRequestedLayer(window->layer);
                    f->setWorkspace(window->workspace);
                    f->setState(WIN_STATE_ALL, window->state);
                    return true;
                }
            }
        }
    }
    return false;
}

bool SMWindows::removeWindowInfo(YFrameWindow */*f*/) {
    return false;
}

SMWindows *sminfo = 0;

static int wr_str(FILE *f, const char *s) {
    if (!s)
        return -1;
    if (fputc('"', f) == EOF)
        return -1;
    while (*s) {
        if ((*s >= '0' && *s <= '9') ||
            (*s >= 'a' && *s <= 'z') ||
            (*s >= 'A' && *s <= 'Z') ||
            *s == '_' ||
            *s == '-' ||
            *s == '.' ||
            *s == ':')
        {
            if (fputc(*s, f) == EOF)
                return -1;
        } else {
            unsigned char c = *s;

            if (fprintf(f, "=%02X", c) != 3)
                return -1;
        }
        s++;
    }
    if (fputc('"', f) == EOF)
        return -1;
    if (fputc(' ', f) == EOF)
        return -1;
    return 0;
}

static int rd_str(char *s, char *d) {
    while (*s == ' ')
        ++s;

    bool quoted = (*s == '"');
    if (quoted)
        ++s;

    for (char c = *s; c; c = *s++) {
        if (quoted ? c == '"' : c == ' ')
            break;
        if (quoted && c == '=') {
            unsigned int i = ' ';
            if (sscanf(s, "%02X", &i) > 0) {
                s += 2;
                c = (char)(i & 0xFF);
            }
        }
        *d++ = c;
    }
    *d = 0;
    return 0;
}

void loadWindowInfo() {
    sminfo = new SMWindows();

    char line[1024];
    char cid[1024];
    char role[1024];
    char klass[1024];
    char instance[1024];
    int x, y, w, h;
    long workspace, layer;
    unsigned long state;
    SMWindowInfo *info;

    upath name = getsesfile();
    FILE *fp = name.fopen("r");
    if (fp == NULL)
        return ;

    while (fgets(line, sizeof(line), fp) != 0) {
        if (line[0] == 'c') {
            if (sscanf(line, "c %s %s %s %d:%d:%d:%d %ld %lu %ld",
                       cid, klass, instance, &x, &y, &w, &h,
                       &workspace, &state, &layer) == 10)
            {
                MSG(("%s %s %s %d:%d:%d:%d %ld %lu %ld",
                    cid, klass, instance, x, y, w, h,
                     workspace, state, layer));
                rd_str(cid, cid);
                rd_str(klass, klass);
                rd_str(instance, instance);
                info = new SMWindowInfo(cid, klass, instance,
                                        x, y, w, h, state, layer, workspace);
                sminfo->addWindowInfo(info);
            } else {
                msg(_("Session Manager: Unknown line %s"), line);
            }
        } else if (line[0] == 'r') {
            if (sscanf(line, "r %s %s %d:%d:%d:%d %ld %lu %ld",
                       cid, role, &x, &y, &w, &h,
                       &workspace, &state, &layer) == 9)
            {
                MSG(("%s %s %s %d:%d:%d:%d %ld %lu %ld\n",
                     cid, klass, instance, x, y, w, h,
                     workspace, state, layer));
                rd_str(cid, cid);
                rd_str(role, role);
                info = new SMWindowInfo(cid, role,
                                        x, y, w, h, state, layer, workspace);
                sminfo->addWindowInfo(info);
            } else {
                msg(_("Session Manager: Unknown line %s"), line);
            }
        } else if (line[0] == 'w') {
            int ws = 0;

            if (sscanf(line, "w %d", &ws) == 1) {
                if (ws >= 0 && ws < manager->workspaceCount())
                    manager->activateWorkspace(ws);
            }
        } else {
            msg(_("Session Manager: Unknown line %s"), line);
        }
    }
    fclose(fp);
}

bool findWindowInfo(YFrameWindow *f) {
    if (sminfo && sminfo->findWindowInfo(f)) {
        return true;
    }
    return false;
}

void YWMApp::smDie() {
    exit(0);
    //!!!manager->exitAfterLastClient(true);
}

void YWMApp::smSaveYourselfPhase2() {
    upath name = getsesfile();
    YFrameWindow *f = 0;
    FILE *fp = name.fopen("w+");
    if (fp == NULL)
        goto end;

    f = manager->topLayer();

    for (; f; f = f->nextLayer()) {
        f->client()->getClientLeader();
        Window leader = f->client()->clientLeader();

        //msg("window=%s", f->client()->windowTitle());
        if (leader != None) {
            //msg("leader=%lX", leader);
            ustring cid = f->client()->getClientId(leader);

            if (cid != null) {
                f->client()->getWindowRole();
                ustring role = f->client()->windowRole();

                if (role != null) {
                    fprintf(fp, "r ");
                    //%s %s ", cid, role);
                    wr_str(fp, cstring(cid).c_str());
                    wr_str(fp, cstring(role).c_str());
                } else {
                    f->client()->getClassHint();
                    char *klass = 0;
                    char *instance = 0;
                    XClassHint *ch = f->client()->classHint();
                    if (ch) {
                        klass = ch->res_class;
                        instance = ch->res_name;
                    }

                    if (klass && instance) {
                        //msg("k=%s, i=%s", klass, instance);
                        fprintf(fp, "c ");
                        //%s %s %s ", cid, klass, instance);
                        wr_str(fp, cstring(cid).c_str());
                        wr_str(fp, cstring(klass).c_str());
                        wr_str(fp, cstring(instance).c_str());
                    } else {
                        continue;
                    }
                }
                fprintf(fp, "%d:%d:%d:%d %d %lu %ld\n",
                        f->x(), f->y(), f->client()->width(), f->client()->height(),
                        f->getWorkspace(), f->getState(), f->getActiveLayer());
            }
        }
    }
    fprintf(fp, "w %lu\n", manager->activeWorkspace());
    fclose(fp);
end:
    YSMApplication::smSaveYourselfPhase2();
}

#endif /* CONFIG_SESSION */

// vim: set sw=4 ts=4 et:
