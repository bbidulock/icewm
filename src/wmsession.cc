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

#include <stdio.h>
#include <string.h>

#include "intl.h"

SMWindowKey::SMWindowKey(YFrameWindow */*f*/) {
}

SMWindowKey::SMWindowKey(char *id, char *role) {
    clientId = newstr(id);
    windowRole = newstr(role);
    windowClass = 0;
    windowInstance = 0;
}

SMWindowKey::SMWindowKey(char *id, char *klass, char *instance) {
    clientId = newstr(id);
    windowRole = 0;
    windowClass = newstr(klass);
    windowInstance = newstr(instance);
}

SMWindowKey::~SMWindowKey() {
}

SMWindowInfo::SMWindowInfo(YFrameWindow *f): key(f) {
}

SMWindowInfo::SMWindowInfo(char *id, char *role,
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

SMWindowInfo::SMWindowInfo(char *id, char *klass, char *instance,
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

    char *cid = f->client()->getClientId(leader);
    if (cid == 0) return false;

    for (unsigned i = 0; i < fWindows.getCount(); ++i) {
        const SMWindowInfo *window = fWindows.getItem(i);

        if (strcmp(cid, window->key.clientId) == 0) {
            if (window->key.windowClass &&
                window->key.windowInstance)
            {
                char *klass = 0;
                char *instance = 0;
                XClassHint *ch = f->client()->classHint();

                if (ch) {
                    klass = ch->res_class;
                    instance = ch->res_name;
                }

                if (strcmp(klass, window->key.windowClass) == 0 &&
                    strcmp(instance, window->key.windowInstance) == 0) {
                    MSG(("got c %s %s %s %d:%d:%d:%d %d %ld %d", 
                         cid, klass, instance,
                         window->x, window->y, window->width, window->height,
                         window->workspace, window->state, window->layer));
                    f->configureClient(window->x, window->y,
                                       window->width, window->height);
                    f->setRequestedLayer(window->layer);
                    f->setWorkspace(window->workspace);
                    f->setState(WIN_STATE_ALL, window->state);

                    XFree(cid);
                    return true;
                }
            }
        }
    }

    XFree(cid);
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
        if (*s >= '0' && *s <= '9' ||
            *s >= 'a' && *s <= 'z' ||
            *s >= 'A' && *s <= 'Z' ||
            *s == '_' || *s == '-' ||
            *s == '.' || *s == ':')
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
    char c;
    bool old = true;

    c = *s++;
    while (c == ' ')
        c = *s++;
    if (c == '"') {
        old = false;
        c = *s++;
    }

    while (c != 0) {
        if (c == '"' && !old) {
            c = *s++;
            break;
        }
        if (c == ' ' && old)
            break;
        if (!old && c == '=') {
            int i = ' ';

            sscanf(s, "%02X", &i);
            s += 2;
            c = i & 0xFF;
        }
        *d++ = c;
        c = *s++;
    }
    *d = 0;
    return 0;
}

void loadWindowInfo() {
    sminfo = new SMWindows();

    FILE *fp;
    char line[1024];
    char cid[1024];
    char role[1024];
    char klass[1024];
    char instance[1024];
    int x, y, w, h;
    long workspace, layer;
    unsigned long state;
    SMWindowInfo *info;

    char *name = getsesfile();

    if (name == 0)
        return ;

    fp = fopen(name, "r");
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
    FILE *fp;
    char *name = getsesfile();
    YFrameWindow *f = 0;
    
    if (name == 0)
        goto end;

    fp = fopen(name, "w+");
    if (fp == NULL)
        goto end;

    f = manager->topLayer();

    for (; f; f = f->nextLayer()) {
        f->client()->getClientLeader();
        Window leader = f->client()->clientLeader();

        //msg("window=%s", f->client()->windowTitle());
        if (leader != None) {
            //msg("leader=%lX", leader);
            char *cid = 0;

            cid = f->client()->getClientId(leader);

            if (cid) {
                f->client()->getWindowRole();
                const char *role = f->client()->windowRole();

                if (role) {
                    fprintf(fp, "r ");
                    //%s %s ", cid, role);
                    wr_str(fp, cid);
                    wr_str(fp, role);
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
                        wr_str(fp, cid);
                        wr_str(fp, klass);
                        wr_str(fp, instance);
                    } else {
                        XFree(cid);
                        continue;
                    }
                }
                XFree(cid);
                fprintf(fp, "%d:%d:%d:%d %ld %lu %ld\n",
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
