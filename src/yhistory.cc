/*
 *  IceWM - History file management
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/09/27: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "config.h"
#include "default.h"
#include "intl.h"

#include "yhistory.h"
#include "ypaths.h"
#include "base.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

YHistory::YHistory(char const *id):
    fId(newstr(id)), fItems(NULL),
    fHead(0), fCount(0), fCommitChanges(true) {
    restore();
}

YHistory::~YHistory(void) {
    for (int n(0); n < historyCapacity; ++n) delete[] fItems[n];
    delete[] fItems;
    delete[] fId;
}

void YHistory::add(char const * str) {
    if (NULL == fItems) {
        fItems = new char *[historyCapacity];
        for (int n(0); n < historyCapacity; ++n) fItems[n] = NULL;
    }

    int const tail((fHead + fCount) % historyCapacity);

    if (!fCount || strcmp (get(tail - 1), str)) {
        delete[] fItems[tail];
        fItems[tail] = newstr(str);

        if (fCount < historyCapacity) ++fCount;
        else ++fHead%= historyCapacity;
        
        commit();
    }
}

char const *YHistory::get(int index) const {
    return index < count() ? fItems[(fHead + index) % historyCapacity] : NULL;
}

char * YHistory::getFilename(char const *id) {
    return YResourcePaths::getPrivateFilename(id, ".history");
}

void YHistory::restore(void) {
    char *filename(getFilename());
    FILE *history(fopen(filename, "r"));

    fCommitChanges = false;
    history = fopen(filename, "r");

    if (NULL != history) {
        char line[1024];

        MSG(("loading history from: %s", filename));
        while (fgets(line, sizeof(line), history)) {
            line[strlen(line)-1] = '\0'; add(line);
        }

        fclose(history);
    }

    fCommitChanges = true;
    delete[] filename;
}

void YHistory::commit(void) {
    if (fCommitChanges) {
    	char *filename(getFilename());
        FILE *history(fopen(filename, "w"));

        if (NULL != history) {
            MSG(("storing history in: %s", filename));
            for (int n(0); n < fCount; ++n) fprintf(history, "%s\n", get(n));
            fclose(history);
        } else
            warn(_("Failed to write history file %s: %s"),
                   filename, strerror(errno));

        delete[] filename;
    }
}
