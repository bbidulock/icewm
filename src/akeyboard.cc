#include "config.h"
#include "akeyboard.h"
#include "wmconfig.h"
#include "default.h"
#include "themable.h"
#include "ymenuitem.h"
#include "ytrace.h"
#include "wmmgr.h"
#include "intl.h"

extern ref<YPixmap> taskbackPixmap;

KeyboardStatus::KeyboardStatus(IAppletContainer* taskBar, YWindow *aParent):
    IApplet(this, aParent),
    taskBar(taskBar),
    fKeyboard(manager->getKeyboard()),
    fFont(YFont::getFont(XFA(tempFontName))),
    fColor(&clrCpuTemp),
    fIndex(0)
{
    if (configKeyboards.nonempty()) {
        int w = (fFont != null ? 4 + fFont->textWidth("MM", 2) : 0);
        int h = (fFont != null ? 4 + fFont->height() : 0);
        setSize(w, h);
    }
    setTitle("Keyboard");
    updateToolTip();
    if (fKeyboard != null) {
        fIcon = YIcon::getIcon(fKeyboard.substring(0, 2));
    }
}

KeyboardStatus::~KeyboardStatus() {
}

void KeyboardStatus::updateKeyboard(mstring keyboard) {
    if (fKeyboard != keyboard) {
        fKeyboard = keyboard;
        if (fKeyboard != null) {
            fIcon = YIcon::getIcon(fKeyboard.substring(0, 2));
        } else {
            fIcon = null;
        }
        repaint();
        updateToolTip();
    }
}

void KeyboardStatus::updateToolTip() {
    if (fKeyboard != null) {
        setToolTip(fKeyboard);
    }
}

void KeyboardStatus::actionPerformed(YAction action, unsigned int modifiers) {
    int index = (action.ident() - actionShow) / 2;
    if (inrange(index, 0, configKeyboards.getCount() - 1)) {
        fIndex = index;
        manager->setKeyboard(fIndex);
    }
    if (fMenu) {
        fMenu = nullptr;
    }
}

void KeyboardStatus::handleClick(const XButtonEvent& up, int count) {
    if (up.button == Button1 && up.type == ButtonRelease) {
        if (++fIndex >= configKeyboards.getCount()) {
            fIndex = 0;
        }
        if (configKeyboards.nonempty()) {
            manager->updateKeyboard(fIndex);
        }
    }
    else if (up.button == Button3 && up.type == ButtonRelease) {
        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("Keyboard"), -2, null, actionNull)->setEnabled(false);
        fMenu->addSeparator();
        for (int i = 0; i < configKeyboards.getCount(); ++i) {
            fMenu->addItem(configKeyboards[i], -2, null,
                           EAction(actionShow + 2 * i))
                            ->setChecked(i == fIndex);
        }
        fMenu->popup(nullptr, nullptr, nullptr, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

bool KeyboardStatus::picture() {
    // bool create = (hasPixmap() == false);

    Graphics G(getPixmap(), width(), height(), depth());
    G.clear();

    fill(G);
    draw(G);

    return true;
}

void KeyboardStatus::fill(Graphics& g) {
    ref<YImage> gradient(getGradient());

    if (gradient != null) {
        g.drawImage(gradient, x(), y(), width(), height(), 0, 0);
    }
    else if (taskbackPixmap != null) {
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height(), x(), y());
    }
    else {
        g.setColor(taskBarBg);
        g.fillRect(0, 0, width(), height());
    }
}

void KeyboardStatus::draw(Graphics& g) {
    if (fFont != null && fKeyboard != null) {
        ref<YImage> icon;
        if (fIcon != null) {
            upath path(fIcon->findIcon(YIcon::smallSize()));
            if (path.nonempty()) {
                YTraceIcon trace(path.string());
                icon = YImage::load(path);
            }
        }
        g.setColor(fColor);
        if (icon != null) {
            unsigned wid = min(width(), icon->width());
            unsigned hei = min(height(), icon->height());
            int dx = int((width() - wid) / 2);
            int dy = int((height() - hei) / 2);
            g.drawImage(icon, 0, 0, wid, hei, dx, dy);
        } else {
            g.setFont(fFont);
            mstring text(fKeyboard.substring(0, 2));
            int y = (height() - 1 - fFont->height()) / 2 + fFont->ascent();
            int w = fFont->textWidth(text);
            int x = max(1, (int(g.rwidth()) - w) / 2);
            g.drawChars(text, x, y);
        }
    }
}

// vim: set sw=4 ts=4 et:
