#define defgKeyWinRaise                 XK_F1, kfAlt, "Alt+F1"
#define defgKeyWinOccupyAll             XK_F2, kfAlt, "Alt+F2"
#define defgKeyWinLower                 XK_F3, kfAlt, "Alt+F3"
#define defgKeyWinClose                 XK_F4, kfAlt, "Alt+F4"
#define defgKeyWinRestore               XK_F5, kfAlt, "Alt+F5"
#define defgKeyWinNext                  XK_F6, kfAlt, "Alt+F6"
#define defgKeyWinPrev                  XK_F6, kfAlt+kfShift, "Alt+Shift+F6"
#define defgKeyWinMove                  XK_F7, kfAlt, "Alt+F7"
#define defgKeyWinSize                  XK_F8, kfAlt, "Alt+F8"
#define defgKeyWinMinimize              XK_F9, kfAlt, "Alt+F9"
#define defgKeyWinMaximize              XK_F10, kfAlt, "Alt+F10"
#define defgKeyWinMaximizeVert          XK_F10, kfAlt+kfShift, "Alt+Shift+F10"
#define defgKeyWinHide                  XK_F11, kfAlt, "Alt+F11"
#define defgKeyWinRollup                XK_F12, kfAlt, "Alt+F12"
#define defgKeySysSwitchNext            XK_Tab, kfAlt, "Alt+Tab"
#define defgKeySysSwitchLast            XK_Tab, kfAlt+kfShift, "Alt+Shift+Tab"
#define defgKeySysWinNext               XK_Escape, kfAlt, "Alt+Esc"
#define defgKeySysWinPrev               XK_Escape, kfAlt+kfShift, "Alt+Shift+Esc"
#define defgKeySysWinMenu               XK_Escape, kfShift, "Shift+Esc"
#define defgKeySysDialog                XK_Delete, kfAlt+kfCtrl, "Alt+Ctrl+Del"
#define defgKeySysMenu                  XK_Escape, kfCtrl, "Ctrl+Esc"
#define defgKeySysWindowList            XK_Escape, kfCtrl+kfAlt, "Alt+Ctrl+Esc"
#define defgKeySysRun                   'r', kfAlt+kfCtrl, "Alt+Ctrl+r"
#define defgKeySysAddressBar            ' ', kfAlt+kfCtrl, "Alt+Ctrl+Space"
#define defgKeyWinMenu                  ' ', kfAlt, "Alt+Space"
#define defgKeySysWorkspacePrev         XK_Left, kfAlt+kfCtrl, "Alt+Ctrl+Left"
#define defgKeySysWorkspaceNext         XK_Right, kfAlt+kfCtrl, "Alt+Ctrl+Right"
#define defgKeySysWorkspacePrevTakeWin  XK_Left, kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+Left"
#define defgKeySysWorkspaceNextTakeWin  XK_Right, kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+Right"
#define defgKeySysWorkspace1            '1', kfAlt+kfCtrl, "Alt+Ctrl+1"
#define defgKeySysWorkspace2            '2', kfAlt+kfCtrl, "Alt+Ctrl+2"
#define defgKeySysWorkspace3            '3', kfAlt+kfCtrl, "Alt+Ctrl+3"
#define defgKeySysWorkspace4            '4', kfAlt+kfCtrl, "Alt+Ctrl+4"
#define defgKeySysWorkspace5            '5', kfAlt+kfCtrl, "Alt+Ctrl+5"
#define defgKeySysWorkspace6            '6', kfAlt+kfCtrl, "Alt+Ctrl+6"
#define defgKeySysWorkspace7            '7', kfAlt+kfCtrl, "Alt+Ctrl+7"
#define defgKeySysWorkspace8            '8', kfAlt+kfCtrl, "Alt+Ctrl+8"
#define defgKeySysWorkspace9            '9', kfAlt+kfCtrl, "Alt+Ctrl+9"
#define defgKeySysWorkspace10           '0', kfAlt+kfCtrl, "Alt+Ctrl+0"
#define defgKeySysWorkspace11           '-', kfAlt+kfCtrl, "Alt+Ctrl+["
#define defgKeySysWorkspace12           '=', kfAlt+kfCtrl, "Alt+Ctrl+]"
#define defgKeySysWorkspace1TakeWin     '1', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+1"
#define defgKeySysWorkspace2TakeWin     '2', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+2"
#define defgKeySysWorkspace3TakeWin     '3', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+3"
#define defgKeySysWorkspace4TakeWin     '4', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+4"
#define defgKeySysWorkspace5TakeWin     '5', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+5"
#define defgKeySysWorkspace6TakeWin     '6', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+6"
#define defgKeySysWorkspace7TakeWin     '7', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+7"
#define defgKeySysWorkspace8TakeWin     '8', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+8"
#define defgKeySysWorkspace9TakeWin     '9', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+9"
#define defgKeySysWorkspace10TakeWin    '0', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+0"
#define defgKeySysWorkspace11TakeWin    '-', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+["
#define defgKeySysWorkspace12TakeWin    '=', kfAlt+kfCtrl+kfShift, "Alt+Ctrl+Shift+]"

#define NO_KEYBIND // !!! fix

#ifdef NO_KEYBIND

#define IS_WMKEYx2(k,vm,k1,vm1,d) ((k) == (k1) && ((vm) == (vm1)))
#define IS_WMKEYx(k,vm,b) IS_WMKEYx2(k,vm,b)
#define IS_WMKEY(k,vm,b) IS_WMKEYx(k,vm,def##b)
#define GRAB_WMKEYx2(k,vm,d) grabVKey(k,vm)
#define GRAB_WMKEYx(k) GRAB_WMKEYx2(k)
#define GRAB_WMKEY(k) GRAB_WMKEYx(def##k)
#define KEY_NAMEx2(k,m,s) (s)
#define KEY_NAMEx(k) KEY_NAMEx2(k)
#define KEY_NAME(k) KEY_NAMEx(def##k)

#else

#ifdef CFGDEF
#define DEF_WMKEY(k) WMKey k = { def##k, true }
#else

#ifdef GENPREF
typedef unsigned int KeySym;
#endif

typedef struct {
    KeySym key;
    int mod;
    const char *name;
    bool initial;
} WMKey;

#define DEF_WMKEY(k) extern WMKey k
#define IS_WMKEY(k,m,b) k == b.key && m == b.mod
#define GRAB_WMKEY(k) GRAB_WMKEYx(k);
#define GRAB_WMKEYx(k) grabVKey(k.key, k.mod)
#define KEY_NAME(k) (k.name ? k.name : "")
#endif

DEF_WMKEY(gKeyWinRaise);
DEF_WMKEY(gKeyWinOccupyAll);
DEF_WMKEY(gKeyWinLower);
DEF_WMKEY(gKeyWinClose);
DEF_WMKEY(gKeyWinRestore);
DEF_WMKEY(gKeyWinPrev);
DEF_WMKEY(gKeyWinNext);
DEF_WMKEY(gKeyWinMove);
DEF_WMKEY(gKeyWinSize);
DEF_WMKEY(gKeyWinMinimize);
DEF_WMKEY(gKeyWinMaximize);
DEF_WMKEY(gKeyWinMaximizeVert);
DEF_WMKEY(gKeyWinHide);
DEF_WMKEY(gKeyWinRollup);
DEF_WMKEY(gKeyWinMenu);
DEF_WMKEY(gKeySysSwitchNext);
DEF_WMKEY(gKeySysSwitchLast);
DEF_WMKEY(gKeySysWinNext);
DEF_WMKEY(gKeySysWinPrev);
DEF_WMKEY(gKeySysWinMenu);
DEF_WMKEY(gKeySysDialog);
DEF_WMKEY(gKeySysMenu);
DEF_WMKEY(gKeySysWindowList);
DEF_WMKEY(gKeySysRun);
DEF_WMKEY(gKeySysAddressBar);
DEF_WMKEY(gKeySysWorkspacePrev);
DEF_WMKEY(gKeySysWorkspaceNext);
DEF_WMKEY(gKeySysWorkspacePrevTakeWin);
DEF_WMKEY(gKeySysWorkspaceNextTakeWin);
DEF_WMKEY(gKeySysWorkspace1);
DEF_WMKEY(gKeySysWorkspace2);
DEF_WMKEY(gKeySysWorkspace3);
DEF_WMKEY(gKeySysWorkspace4);
DEF_WMKEY(gKeySysWorkspace5);
DEF_WMKEY(gKeySysWorkspace6);
DEF_WMKEY(gKeySysWorkspace7);
DEF_WMKEY(gKeySysWorkspace8);
DEF_WMKEY(gKeySysWorkspace9);
DEF_WMKEY(gKeySysWorkspace10);
DEF_WMKEY(gKeySysWorkspace11);
DEF_WMKEY(gKeySysWorkspace12);
DEF_WMKEY(gKeySysWorkspace1TakeWin);
DEF_WMKEY(gKeySysWorkspace2TakeWin);
DEF_WMKEY(gKeySysWorkspace3TakeWin);
DEF_WMKEY(gKeySysWorkspace4TakeWin);
DEF_WMKEY(gKeySysWorkspace5TakeWin);
DEF_WMKEY(gKeySysWorkspace6TakeWin);
DEF_WMKEY(gKeySysWorkspace7TakeWin);
DEF_WMKEY(gKeySysWorkspace8TakeWin);
DEF_WMKEY(gKeySysWorkspace9TakeWin);
DEF_WMKEY(gKeySysWorkspace10TakeWin);
DEF_WMKEY(gKeySysWorkspace11TakeWin);
DEF_WMKEY(gKeySysWorkspace12TakeWin);

#undef DEF_WMKEY

#endif
