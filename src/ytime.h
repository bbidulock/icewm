#ifndef YTIME_H
#define YTIME_H

#include <sys/time.h>

timeval walltime();
timeval monotime();
timeval millitime(long msec);
timeval maketime(long sec, long usec);
long seconds();
bool fsleep(double delay);

inline bool operator<(const timeval& a, const timeval& b) {
    return a.tv_sec != b.tv_sec ?
           a.tv_sec  < b.tv_sec :
           a.tv_usec < b.tv_usec;
}

inline bool operator>=(const timeval& a, const timeval& b) {
    return !(a < b);
}

inline bool operator>(const timeval& a, const timeval& b) {
    return b < a;
}

inline bool operator<=(const timeval& a, const timeval& b) {
    return !(b < a);
}

inline bool operator==(const timeval& a, const timeval& b) {
    return a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec;
}

inline timeval operator+(const timeval& a, const timeval& b) {
    return maketime( a.tv_sec + b.tv_sec, a.tv_usec + b.tv_usec );
}

inline timeval operator+(const timeval& a, long sec) {
    return maketime( a.tv_sec + sec, a.tv_usec );
}

inline timeval operator-(const timeval& a, const timeval& b) {
    return maketime( a.tv_sec - b.tv_sec, a.tv_usec - b.tv_usec );
}

inline timeval& operator+=(timeval& a, const timeval& b) {
    return a = a + b;
}

inline double toDouble(const timeval& t) {
    return double(t.tv_sec) + 1e-6 * double(t.tv_usec);
}

inline timeval zerotime() { return (timeval) { 0L, 0L }; }

#endif

// vim: set sw=4 ts=4 et:
