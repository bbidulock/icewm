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
#include "yhistory.h"

char * YHistory::getFilename(char const *id) {
    return YResourcePaths::getPrivateFilename(id, ".history");
}
