/*
 *  FDOmenu - Menu code generator for icewm
 *  Copyright (C) 2015 Eduard Bloch
 *
 *  Inspired by icewm-menu-gnome2 and Freedesktop.org specifications
 *  Using pure glib/gio code and a built-in menu structure instead
 *  the XML based external definition (as suggested by FD.o specs)
 *
 *  Release under terms of the GNU Library General Public License
 *  (version 2.0)
 *
 *  2015/02/05: Eduard Bloch <edi@gmx.de>
 *  - initial version
 */

#include "config.h"
#include "base.h"
#include "sysdep.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>

template<class T>
struct auto_gfree
{
	T *p;
	auto_gfree(T *xp) : p(xp) {};
	~auto_gfree() { g_free(p); }
};

//#warning needing a dupe filter for filename, maybe use GHashTable for that

void proc_dir(const char *path, int depth=7)
{
	if(!--depth)
		return;

	//printf("dir: %s\n", path);

	GDir *pdir = g_dir_open (path, 0, NULL);
	if(!pdir)
		return;

	const gchar *szFilename(NULL);
	while(NULL != (szFilename = g_dir_read_name (pdir)))
	{
		if(!szFilename)
			continue;
		gchar *szFullName = g_strjoin("/", path, szFilename, 0);
		auto_gfree<gchar> xxfree(szFullName);
		static GStatBuf buf;
		if(g_stat(szFullName, &buf))
			return;
		if(S_ISDIR(buf.st_mode))
			proc_dir(szFullName, depth);

		if(!S_ISREG(buf.st_mode))
			return;
		//printf("got: %s\n", szFullName);

		GDesktopAppInfo *pInfo = g_desktop_app_info_new_from_filename (szFullName);
		if(!pInfo)
			continue;

		if(!g_app_info_should_show((GAppInfo*) pInfo))
			continue;

		const char *cmdraw = g_app_info_get_commandline ((GAppInfo*) pInfo);
		if(!cmdraw || !*cmdraw)
			continue;

		// let's whitespace all special fields that we don't support
		gchar * cmd = g_strdup(cmdraw);
		auto_gfree<gchar> cmdfree(cmd);
		bool bDelChars=false;
		for(gchar *p=cmd; *p; ++p)
		{
			if('%' == *p)
				bDelChars=true;
			if(bDelChars)
				*p = ' ';
			if(bDelChars && !isprint((unsigned)*p))
				bDelChars=false;
		}

		const char *pName=g_app_info_get_name( (GAppInfo*) pInfo);
		if(!pName)
			continue;
		const char *pCats=g_desktop_app_info_get_categories(pInfo);
		if(!pCats)
			pCats="Other";


		//printf("icon: %s -> %s\n", pName, pCats);

		GIcon *pIcon=g_app_info_get_icon( (GAppInfo*) pInfo);
		const char *sicon = "-";
		if (pIcon)
		{
			sicon = g_icon_to_string(pIcon);
			//printf("got: %s -> %s\n", pName, sicon);
		}
		g_print("# %s\nprog \"%s\" %s %s\n", szFullName, pName, sicon, cmd);
	}
	g_dir_close(pdir);
}

int main(int argc, char **) {

	setlocale (LC_ALL, "");

	const char * usershare=getenv("XDG_DATA_HOME"),
			*sysshare=getenv("XDG_DATA_DIRS");

	if(!usershare || !*usershare)
		usershare=g_strjoin(NULL, getenv("HOME"), "/.local/share", NULL);

	if(!sysshare || !*sysshare)
		sysshare="/usr/local/share/:/usr/share/";

	if(argc>1)
	{
		g_fprintf(stderr, "This program doesn't use command line options. It only listens to\n"
			"environment variables defined by XDG Base Directory Specification.\n"
			"XDG_DATA_HOME=%s\nXDG_DATA_DIRS=%s\n"
			,usershare, sysshare);
		return EXIT_FAILURE;
	}

	proc_dir(usershare);
	gchar **ppDirs = g_strsplit (sysshare, ":", -1);
	for(const gchar * const * p=ppDirs;*p;++p)
		proc_dir(g_strjoin(0, *p, "/applications", 0));

	return EXIT_SUCCESS;
}
