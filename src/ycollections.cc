/*
 * Copyright (c) 2017 Eduard Bloch
 * License: WTFPL
 *
 * Collection of various data structures used to support certain IceWM functionality.
 * All of them are made for simple data structures which must be trivially copyable and mem-movable!
 */

#include "config.h"
#include <string.h>

#include "ycollections.h"

// ensure explicit instantiation of some helpers

template <>
inline bool lessThan<char const*>(char const* a, char const* b)
{
    return strcmp(a ? a : "", b ? b : "") < 0;
}

template class
YSortedMap<const char*, int>;

