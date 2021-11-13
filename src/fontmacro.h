#ifndef FONT_MACRO_H
#define FONT_MACRO_H

#ifndef CFGDIR
#error include config.h
#endif

#if !defined(FONTS_ADOBE) \
 && !defined(FONTS_LUCIDA) \
 && !defined(FONTS_DEJAVU) \
 && !defined(FONTS_DROID)
#define FONTS_DEJAVU 1
// #define FONTS_ADOBE 1
// #define FONTS_LUCIDA 1
// #define FONTS_DROID 1
#endif

#define FONT2(pf,px,pt)     pf #px "-" #pt "-*-*-*-*-*-*"       // "iso10646-1"

#if defined(FONTS_DEJAVU)
#define FONT(px,pt)          FONT2("-misc-dejavu sans-medium-r-normal--", px, *)
#define BOLDFONT(px,pt)        FONT2("-misc-dejavu sans-bold-r-normal--", px, *)
#define TTFONT(px,pt)   FONT2("-misc-dejavu sans mono-medium-r-normal--", px, *)
#define BOLDTTFONT(px,pt) FONT2("-misc-dejavu sans mono-bold-r-normal--", px, *)

#elif defined(FONTS_ADOBE)
#define FONT(px,pt)   FONT2("-adobe-helvetica-medium-r-normal--", px, pt)
#define BOLDFONT(px,pt) FONT2("-adobe-helvetica-bold-r-normal--", px, pt)
#define TTFONT(px,pt)   FONT2("-adobe-courier-medium-r-normal--", px, pt)
#define BOLDTTFONT(px,pt) FONT2("-adobe-courier-bold-r-normal--", px, pt)

#elif defined(FONTS_LUCIDA)
#define FONT(px,pt)     FONT2("-b&h-lucida-medium-r-normal-*-", px, pt)
#define BOLDFONT(px,pt) FONT2("-b&h-lucida-bold-r-normal-*-", px, pt)
#define TTFONT(px,pt)   FONT2("-b&h-lucidatypewriter-medium-r-normal-*-", px, pt)
#define BOLDTTFONT(px,pt) FONT2("-b&h-lucidatypewriter-bold-r-normal-*-", px, pt)

#elif defined(FONTS_DROID)
#define FONT(px,pt)     FONT2("-misc-droid sans-medium-r-normal--", px, *)
#define BOLDFONT(px,pt)   FONT2("-misc-droid sans-bold-r-normal--", px, *)
#define TTFONT(px,pt)   FONT2("-misc-droid sans mono-medium-r-normal--", px, *)
#define BOLDTTFONT(px,pt) FONT2("-misc-droid sans mono-bold-r-normal--", px, *)

#endif

#endif
