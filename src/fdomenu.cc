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
#include "intl.h"

const char *g_argv0;

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>

typedef GList* pglist;

template<class T>
struct auto_gfree
{
	T *m_p;
	auto_gfree() : m_p(NULL) {};
	auto_gfree(T *xp) : m_p(xp) {};
	~auto_gfree() { g_free(m_p); }
};
struct auto_gunref
{
	GObject *m_p;
	auto_gunref(GObject *xp): m_p(xp) {};
	~auto_gunref() { g_object_unref(m_p); }
};

bool find_in_zArray(const char * const *arrToScan, const char *keyword)
{
	for(const gchar * const * p=arrToScan;*p;++p)
		if(!strcmp(keyword, *p))
			return true;
	return false;
}

// for optional bits that are not consuming much and
// it's OK to leak them, OS will clean up soon enough
//#define FREEASAP
#ifdef FREEASAP
#define opt_g_free(x) g_free(x);
#else
#define opt_g_free(x)
#endif

pglist msettings=0, mscreensavers=0, maccessories=0, mdevelopment=0, meducation=0,
		mgames=0, mgraphics=0, mmultimedia=0, mnetwork=0, moffice=0, msystem=0,
		mother=0, mwine=0, meditors=0;

//#warning needing a dupe filter for filename, maybe use GHashTable for that

void proc_dir(const char *path, unsigned depth=0)
{
	GDir *pdir = g_dir_open (path, 0, NULL);
	if(!pdir)
		return;
	struct tdircloser {
		GDir *m_p;
		tdircloser(GDir *p) : m_p(p) {};
		~tdircloser() { g_dir_close(m_p);}
	} dircloser(pdir);

	const gchar *szFilename(NULL);
	while(NULL != (szFilename = g_dir_read_name (pdir)))
	{
		if(!szFilename)
			continue;
		gchar *szFullName = g_strjoin("/", path, szFilename, NULL);
		auto_gfree<gchar> xxfree(szFullName);
		static GStatBuf buf;
		if(g_stat(szFullName, &buf))
			return;
		if(S_ISDIR(buf.st_mode))
		{
			static ino_t reclog[6];
			for(unsigned i=0; i<depth; ++i)
			{
				if(reclog[i] == buf.st_ino)
					goto dir_visited_before;
			}
			if(depth<ACOUNT(reclog))
			{
				reclog[++depth] = buf.st_ino;
				proc_dir(szFullName, depth);
				--depth;
			}
			dir_visited_before:;
		}

		if(!S_ISREG(buf.st_mode))
			continue;

		GDesktopAppInfo *pInfo = g_desktop_app_info_new_from_filename (szFullName);
		if(!pInfo)
			continue;
		auto_gunref ___pinfo_releaser((GObject*)pInfo);

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
		if(0 == strncmp(pCats, "X-", 2))
			continue;

		const char *sicon = "-";
		GIcon *pIcon=g_app_info_get_icon( (GAppInfo*) pInfo);
		auto_gfree<char> iconstringrelease;
		if (pIcon)
		{
			char *s = g_icon_to_string(pIcon);
			iconstringrelease.m_p=s;
			sicon=s;
		}

		gchar *menuLine;

		if(g_desktop_app_info_get_boolean (pInfo,
                "Terminal") || strchr(cmdraw, '%'))
		{
			menuLine = g_strdup_printf(
					"prog \"%s\" %s %s \"%s\"\n",
					pName, sicon, g_argv0, szFullName);
		}
		else
			menuLine = g_strdup_printf(
				"prog \"%s\" %s %s\n",
				pName, sicon, cmd);

		// Pigeonholing roughly by guessed menu structure
#define add2menu(x) { x=g_list_append(x, menuLine); }
		gchar **ppCats = g_strsplit(pCats, ";", -1);
		if (find_in_zArray(ppCats, "Screensaver"))
			add2menu(mscreensavers)
		else if (find_in_zArray(ppCats, "Settings"))
			add2menu(msettings)
		else if (find_in_zArray(ppCats, "Accessories"))
			add2menu(maccessories)
		else if (find_in_zArray(ppCats, "Development"))
			add2menu(mdevelopment)
		else if (find_in_zArray(ppCats, "Education"))
			add2menu(meducation)
		else if (find_in_zArray(ppCats, "Game"))
			add2menu(mgames)
		else if (find_in_zArray(ppCats, "Graphics"))
			add2menu(mgraphics)
		else if (find_in_zArray(ppCats, "AudioVideo") || find_in_zArray(ppCats, "Audio")
				|| find_in_zArray(ppCats, "Video"))
		{
			add2menu(mmultimedia)
		}
		else if (find_in_zArray(ppCats, "Network"))
			add2menu(mnetwork)
		else if (find_in_zArray(ppCats, "Office"))
			add2menu(moffice)
		else if (find_in_zArray(ppCats, "System") || find_in_zArray(ppCats, "Emulator"))
			add2menu(msystem)
		else if (strstr(pCats, "Editor"))
					add2menu(meditors)
		else
		{
			const char *pwmclass = g_desktop_app_info_get_startup_wm_class(pInfo);
			if ((pwmclass && strstr(pwmclass, "Wine")) || strstr(cmd, " wine "))
				add2menu(mwine)
			else
				add2menu(mother)
		}
		g_strfreev(ppCats);
	}
}

struct tMenuHead {
		char *title;
		pglist pEntries;
		tMenuHead(char *xt, pglist p) : title(xt), pEntries(p) {};
	};
gint menu_name_compare(gconstpointer a, gconstpointer b)
{
	tMenuHead *pa=(tMenuHead*)a;
	tMenuHead *pb=(tMenuHead*)b;
	return g_utf8_collate(pa->title, pb->title);
}

void print_submenu(gpointer vlp)
{
	tMenuHead *l=static_cast<tMenuHead*>(vlp);
	if(!l->pEntries)
		return;
	l->pEntries=g_list_sort(l->pEntries, (GCompareFunc)g_utf8_collate);
	printf("menu \"%s\" folder {\n", l->title);
	for (pglist m = l->pEntries; m != NULL; m = m->next)
	{
		printf("%s", (const char*) m->data);
		g_free(m->data);
	}
	printf("}\n");
}

void dump_menu()
{
	pglist xmenu = 0;
#define	addmenu(name, store) \
		xmenu = g_list_insert_sorted(xmenu, new tMenuHead(name, store),\
				(GCompareFunc)menu_name_compare);
	addmenu(_("Settings"), msettings);
	addmenu(_("Screensavers"), mscreensavers);
	addmenu(_("Accessories"), maccessories);
	addmenu(_("Development"), mdevelopment);
	addmenu(_("Education"), meducation);
	addmenu(_("Games"), mgames);
	addmenu(_("Graphics"), mgraphics);
	addmenu(_("Multimedia"), mmultimedia);
	addmenu(_("Network"), mnetwork);
	addmenu(_("Office"), moffice);
	addmenu(_("System"), msystem);
	addmenu(_("WINE"), mwine);
	addmenu(_("Editors"), meditors);

	for (pglist l = xmenu; l != NULL; l = l->next)
		print_submenu(l->data);
	puts("separator");
	tMenuHead hmo(_("Other"), mother);
	print_submenu(&hmo);

}

bool launch(const char *dfile)
{
	GDesktopAppInfo *pInfo = g_desktop_app_info_new_from_filename (dfile);
	if(!pInfo)
		return false;
	return g_app_info_launch ((GAppInfo *)pInfo,
                   NULL, NULL, NULL);
}

int main(int argc, char **argv)
{
	g_argv0=argv[0];

	setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

	const char * usershare=getenv("XDG_DATA_HOME"),
			*sysshare=getenv("XDG_DATA_DIRS");

	if(!usershare || !*usershare)
		usershare=g_strjoin(NULL, getenv("HOME"), "/.local/share", NULL);

	if(!sysshare || !*sysshare)
		sysshare="/usr/local/share:/usr/share";

	if(argc>1)
	{
		if(strstr(argv[1], ".desktop") && launch(argv[1]))
			return EXIT_SUCCESS;

		g_fprintf(stderr, "This program doesn't use command line options. It only listens to\n"
			"environment variables defined by XDG Base Directory Specification.\n"
			"XDG_DATA_HOME=%s\n"
				"XDG_DATA_DIRS=%s\n"
			,usershare, sysshare);
		return EXIT_FAILURE;
	}

	proc_dir(usershare);
	gchar **ppDirs = g_strsplit (sysshare, ":", -1);
	for(const gchar * const * p=ppDirs;*p;++p)
	{
		gchar *pmdir=g_strjoin(0, *p, "/applications", NULL);
		proc_dir(pmdir);
		opt_g_free(pmdir);
	}
#ifdef FREEASAP
	g_strfreev(ppDirs);
#endif

	dump_menu();

	return EXIT_SUCCESS;
}
