#ifndef __YSTYLE_H
#define __YSTYLE_H

class YStyle {
public:
    YStyle();
    ~YStyle();

    void drawBg(Graphics &g, QRect &r);
    void drawText(Graphics &g, QRect &r, const char *text);

private:
    YColor *fBackgrouncColor;
    YColor *fForegroundColor;
    YPixmap *fPixmap;
    int fBorderStyle;
};

#endif
