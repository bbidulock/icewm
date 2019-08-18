/**
 *
 * Copyright (C) 1995 Free Software Foundation, Inc.
 *
 * This file is part of the GNU LessTif Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **/

#ifndef __MWMUTIL_H
#define __MWMUTIL_H

#define PROP_MWM_HINTS_ELEMENTS  5

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
#define MWM_HINTS_MASK          15L

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
#define MWM_FUNC_MASK           63L

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
#define MWM_DECOR_MASK          127L

#define MWM_INPUT_MODELESS               0L
#define MWM_INPUT_APPLICATION_MODAL      1L
#define MWM_INPUT_SYSTEM_MODAL           2L
#define MWM_INPUT_FULL_APPLICATION_MODAL 3L

#define MWM_TEAROFF_WINDOW      (1L<<0)

struct MwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;

    MwmHints(unsigned long flags = 0,
             unsigned long functions = 0,
             unsigned long decorations = 0,
             long input_mode = 0,
             unsigned long status = 0):
        flags(flags),
        functions(functions),
        decorations(decorations),
        input_mode(input_mode),
        status(status)
    { }

    bool hasFlags() const { return flags & MWM_HINTS_MASK; }

    bool hasFuncs() const { return flags & MWM_HINTS_FUNCTIONS; }
    bool hasDecor() const { return flags & MWM_HINTS_DECORATIONS; }
    bool hasInput() const { return flags & MWM_HINTS_INPUT_MODE; }
    bool hasStatus() const { return flags & MWM_HINTS_STATUS; }
    bool onlyFuncs() const { return hasFuncs() && !hasDecor(); }

    void setFuncs() { flags |= MWM_HINTS_FUNCTIONS; }
    void setDecor() { flags |= MWM_HINTS_DECORATIONS; }
    void setInput() { flags |= MWM_HINTS_INPUT_MODE; }
    void setStatus() { flags |= MWM_HINTS_STATUS; }

    void notFuncs() { flags &= ~MWM_HINTS_FUNCTIONS; }
    void notDecor() { flags &= ~MWM_HINTS_DECORATIONS; }
    void notInput() { flags &= ~MWM_HINTS_INPUT_MODE; }
    void notStatus() { flags &= ~MWM_HINTS_STATUS; }
};

/*
 * atoms
 */
#define _XA_MOTIF_BINDINGS              "_MOTIF_BINDINGS"
#define _XA_MOTIF_WM_HINTS              "_MOTIF_WM_HINTS"
#define _XA_MOTIF_WM_MESSAGES           "_MOTIF_WM_MESSAGES"
#define _XA_MOTIF_WM_OFFSET             "_MOTIF_WM_OFFSET"
#define _XA_MOTIF_WM_MENU               "_MOTIF_WM_MENU"
#define _XA_MOTIF_WM_INFO               "_MOTIF_WM_INFO"
#define _XA_MWM_HINTS                   _XA_MOTIF_WM_HINTS
#define _XA_MWM_MESSAGES                _XA_MOTIF_WM_MESSAGES
#define _XA_MWM_MENU                    _XA_MOTIF_WM_MENU
#define _XA_MWM_INFO                    _XA_MOTIF_WM_INFO

#endif

// vim: set sw=4 ts=4 et:
