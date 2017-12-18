#include "config.h"

#if defined CONFIG_XPM

#include <math.h>
#include <errno.h>
#include "yimage.h"
#include "yxapp.h"

#include <X11/xpm.h>

#ifdef CONFIG_LIBPNG
#include <png.h>
#endif

class YXImage: public YImage {
public:
    YXImage(XImage *ximage) : YImage(ximage->width, ximage->height), fImage(ximage) {
        // tlog("created YXImage %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
    }
    virtual ~YXImage() {
        if (fImage != 0)
            XDestroyImage(fImage);
    }
    virtual ref<YPixmap> renderToPixmap(unsigned depth);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual bool valid() const { return fImage != 0; }
    unsigned depth() const { return fImage ? fImage->depth : 0; }
    static ref<YImage> loadxbm(upath filename);
    static ref<YImage> loadxpm(upath filename);
#ifdef CONFIG_LIBPNG
    static ref<YImage> loadpng(upath filename);
#endif
    static ref<YImage> combine(XImage *xdraw, XImage *xmask);

private:
    bool hasAlpha() const { return depth() == 32; }
    virtual ref<YImage> upscale(unsigned width, unsigned height);
    virtual ref<YImage> downscale(unsigned width, unsigned height);
    virtual ref<YImage> subimage(int x, int y, unsigned width, unsigned height);
    XImage *fImage;
};

ref<YImage> YImage::create(unsigned width, unsigned height)
{
    ref<YImage> image;
    XImage *ximage = 0;

    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, width, height, 8, 0);
    if (ximage != 0 && (ximage->data = (char*) calloc(ximage->bytes_per_line*height,sizeof(char)))) {
        image.init(new YXImage(ximage));
        ximage = 0; // consumed above
    }
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YImage::load(upath filename)
{
    ref<YImage> image;
    pstring ext(filename.getExtension());

    if (ext == ".xbm")
        image = YXImage::loadxbm(filename);
    else if (ext == ".xpm")
        image = YXImage::loadxpm(filename);
#ifdef CONFIG_LIBPNG
    else if (ext == ".png")
        image = YXImage::loadpng(filename);
#endif
    return image;
}

ref<YImage> YXImage::loadxbm(upath filename)
{
    ref<YImage> image;
    XImage *ximage = 0;
    unsigned w, h;
    int x, y;
    char *data = 0;
    int status;

    status = XReadBitmapFileData(filename.string(), &w, &h, (unsigned char **)&data, &x, &y);
    if (status == BitmapSuccess) {
        ximage = XCreateImage(xapp->display(), None, 1, XYBitmap, 0, data, w, h, 8, 0);
        if (ximage != 0)
            image.init(new YXImage(ximage));
        else
            free(data);
    }
    return image;
}

ref<YImage> YXImage::loadxpm(upath filename)
{
    ref<YImage> image;
    XImage *xdraw = 0, *xmask = 0;
    XpmAttributes xa;
    int status;

    xa.visual = xapp->visual();
    xa.colormap = xapp->colormap();
    xa.depth = xapp->depth();
    xa.valuemask = XpmVisual|XpmColormap|XpmDepth;

    status = XpmReadFileToImage(xapp->display(), filename.string(), &xdraw, &xmask, &xa);

    if (status == XpmSuccess) {
        XpmFreeAttributes(&xa);
        if (xmask == 0)
            image.init(new YXImage(xdraw));
        else {
            XImage *ximage;
            unsigned w = xdraw->width;
            unsigned h = xdraw->height;

            ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, w, h, 8, 0);
            if (ximage && (ximage->data = (char*) calloc(ximage->bytes_per_line*h, sizeof(char)))) {
                for (unsigned j = 0; j < h; j++) {
                    for (unsigned i = 0; i < w; i++) {
                        if (XGetPixel(xmask, i, j))
                            XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) | 0xFF000000);
                        else
                            XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) & 0x00FFFFFF);
                    }
                }
                image.init(new YXImage(ximage));
                ximage = 0; // consumed above
            }
            if (ximage)
                XDestroyImage(ximage);
            XDestroyImage(xdraw);
            XDestroyImage(xmask);
        }
    }
    return image;
}

#ifdef CONFIG_LIBPNG
ref<YImage> YXImage::loadpng(upath filename)
{
    ref<YImage> image;
    png_byte *png_pixels, **row_pointers, *p;
    XImage *ximage = 0;
    // working around clobbering issues with high optimization levels, see https://stackoverflow.com/questions/7721854/what-sense-do-these-clobbered-variable-warnings-make
    static void *vol_png_pixels, *vol_row_pointers;
    static void *vol_ximage;
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte buf[8];
    int ret;
    FILE *f;

    png_pixels = 0;
    row_pointers = 0;
    vol_png_pixels = 0;
    vol_row_pointers = 0;
    vol_ximage = 0;

    if (!(f = fopen(filename.string(), "rb"))) {
        tlog("could not open %s: %s\n", filename.string().c_str(), strerror(errno));
        goto nofile;
    }
    if ((ret = fread(buf, 1, 8, f)) != 8) {
        tlog("could not read PNG signature %s: %s\n", filename.string().c_str(), strerror(errno));
        goto noread;
    }
    if (png_sig_cmp(buf, 0, 8)) {
        tlog("invalid PNG signature in %s\n", filename.string().c_str());
        goto noread;
    }
    if (!(png_ptr = png_create_read_struct(png_get_libpng_ver(NULL), NULL, NULL, NULL))) {
        tlog("could not allocate PNG read struture\n");
        goto noread;
    }
    if (!(info_ptr = png_create_info_struct(png_ptr))) {
        tlog("could not allocate PNG info struture\n");
        goto noinfo;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        tlog("ERROR: longjump from setjump\n");
        goto pngerr;
    }

    png_uint_32 width, height, row_bytes, i, j;
    int bit_depth, color_type, channels;
    unsigned long pixel, A, R, G, B;
    A = R = G = B = 0;
    png_pixels = 0;
    row_pointers = 0;
    vol_png_pixels = 0;
    vol_row_pointers = 0;

    png_init_io(png_ptr, f);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_expand(png_ptr);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
    if (color_type == PNG_COLOR_TYPE_GRAY)
        channels = 1;
    else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        channels = 2;
    else if (color_type == PNG_COLOR_TYPE_RGB)
        channels = 3;
    else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        channels = 4;
    else
        channels = 0;
    row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    png_pixels = new png_byte[row_bytes * height];
    vol_png_pixels = png_pixels;
    row_pointers = new png_bytep[height];
    vol_row_pointers = row_pointers;
    for (i = 0; i < height; i++)
        row_pointers[i] = png_pixels + i * row_bytes;
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, info_ptr);
    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, width, height, 8, 0);
    if (!ximage || !(ximage->data = (char*) calloc(ximage->bytes_per_line*height, sizeof(char)))) {
        if (ximage) {
            tlog("could not allocate ximage data\n");
            XDestroyImage(ximage);
            goto pngerr;
        }
        tlog("could not allocate ximage\n");
        goto pngerr;
    }
    vol_ximage = ximage;
    for (p = png_pixels, j = 0; j < height; j++) {
        for (i = 0; i < width; i++, p += channels) {
            switch(color_type) {
                case PNG_COLOR_TYPE_GRAY:
                    R = G = B = p[0];
                    A = 255;
                    break;
                case PNG_COLOR_TYPE_GRAY_ALPHA:
                    R = G = B = p[0];
                    A = p[1];
                    break;
                case PNG_COLOR_TYPE_RGB:
                    R = p[0];
                    G = p[1];
                    B = p[2];
                    A = 255;
                    break;
                case PNG_COLOR_TYPE_RGB_ALPHA:
                    R = p[0];
                    G = p[1];
                    B = p[2];
                    A = p[3];
                    break;
            }
            pixel = (A << 24)|(R <<16)|(G<<8)|(B<<0);
            XPutPixel(ximage, i, j, pixel);
        }
    }
  pngerr:
    ximage = (XImage *) vol_ximage;
    png_pixels = (png_byte *) vol_png_pixels;
    delete[] png_pixels;
    row_pointers = (png_byte **) vol_row_pointers;
    delete[] row_pointers;
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    goto noread;
  noinfo:
    png_destroy_read_struct(&png_ptr, NULL, NULL);
  noread:
    fclose(f);
  nofile:
    if (ximage)
        image.init(new YXImage(ximage));
    return image;
}
#endif

ref<YImage> YXImage::upscale(unsigned nw, unsigned nh)
{
    ref<YImage> image;
    XImage *ximage = 0;
    double *chanls = 0;
    double *counts = 0;
    double *colors = 0;

    // tlog("upscale from %ux%ux%u to %ux%u\n", width(), height(), depth(), nw, nh);
    if (!valid()) {
        tlog("not a valid YXImage\n");
        goto error;
    }
    {
        unsigned w = width();
        unsigned h = height();
        unsigned d = depth();
        Visual *v = xapp->visual();
        bool has_alpha = hasAlpha();

        ximage = XCreateImage(xapp->display(), v, d, ZPixmap, 0, NULL, nw, nh, 8, 0);
        if (!ximage || !(ximage->data = (char*) calloc(ximage->bytes_per_line*nh, sizeof(char)))) {
            tlog("could not allocate ximage %ux%ux%u or data\n", nw, nh, d);
            goto error;
        }
        // tlog("created upscale ximage at %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        if (!(chanls = new double[ximage->bytes_per_line * ximage->height * 4 * sizeof(*chanls)])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        if (!(counts = new double[ximage->bytes_per_line * ximage->height * sizeof(*counts)])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        if (!(colors = new double[ximage->bytes_per_line * ximage->height * sizeof(*colors)])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        {
            double pppx = (double) w / (double) nw;
            double pppy = (double) h / (double) nh;

            double ty, by; unsigned l;
            for (ty = 0.0, by = pppy, l = 0; l < nh; l++, ty += pppy, by += pppy) {
                for (unsigned j = floor(ty); j < by; j++) {
                    double yf = 1.0;
                    if (ty < (j + 1) && (j + 1) < by)
                        yf = (j + 1) - ty;
                    else if (ty < j && j < by)
                        yf = by - j;
                    double lx, rx; unsigned k;
                    for (lx = 0.0, rx = pppx, k = 0; k < nw; k++, lx += pppx, rx += pppx) {
                        for (unsigned i = floor(lx); i < rx; i++) {
                            unsigned xf = 1.0;
                            if (lx < (i + 1) && (i + 1) < rx)
                                xf = (i + 1) - lx;
                            else if (lx < i && i < rx)
                                xf = rx - i;
                            double ff = xf * yf;
                            unsigned m = l * nw + k;
                            unsigned n = m << 2;
                            counts[m] += ff;
                            unsigned long pixel = XGetPixel(fImage, i, j);
                            unsigned A = has_alpha ? (pixel >> 24) & 0xff : 255;
                            unsigned R = (pixel >> 16) & 0xff;
                            unsigned G = (pixel >>  8) & 0xff;
                            unsigned B = (pixel >>  0) & 0xff;
                            colors[m] += ff;
                            chanls[n+0] += A * ff;
                            chanls[n+1] += R * ff;
                            chanls[n+2] += G * ff;
                            chanls[n+3] += B * ff;
                        }
                    }
                }
            }
            unsigned amax = 0;
            for (unsigned l = 0; l < nh; l++) {
                for (unsigned k = 0; k < nw; k++) {
                    unsigned m = l * nw + k;
                    unsigned n = m << 2;
                    unsigned long pixel = 0;
                    if (counts[m])
                        pixel |= (lround(chanls[n+0] / counts[m]) & 0xff) << 24;
                    if (colors[m]) {
                        pixel |= (lround(chanls[n+1] / colors[m]) & 0xff) << 16;
                        pixel |= (lround(chanls[n+2] / colors[m]) & 0xff) <<  8;
                        pixel |= (lround(chanls[n+3] / colors[m]) & 0xff) <<  0;
                    }
                    XPutPixel(ximage, k, l, pixel);
                    amax = max(amax, (unsigned)((pixel >> 24) & 0xff));
                }
            }
            if (!amax)
                /* no opacity at all! */
                for (unsigned l = 0; l < nh; l++)
                    for (unsigned k = 0; k < nw; k++)
                        XPutPixel(ximage, k, l, XGetPixel(ximage, k, l) | 0xFF000000);
            else if (amax < 255) {
                double bump = (double) 255 / (double) amax;
                for (unsigned l = 0; l < nh; l++)
                    for (unsigned k = 0; k < nw; k++) {
                        unsigned long pixel = XGetPixel(ximage, k, l);
                        amax = (pixel >> 24) & 0xff;
                        amax = lround((double) amax * bump);
                        if (amax > 255)
                            amax = 255;
                        pixel = (pixel & 0x00FFFFFF) | (amax << 24);
                        XPutPixel(ximage, k, l, pixel);
                    }
            }
            image.init(new YXImage(ximage));
            ximage = 0; // consumed above
        }
    }
  error:
    if (chanls)
        delete[] chanls;
    if (counts)
        delete[] counts;
    if (colors)
        delete[] colors;
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YXImage::downscale(unsigned nw, unsigned nh)
{
    ref<YImage> image;
    XImage *ximage = 0;
    double *chanls = 0;
    double *counts = 0;
    double *colors = 0;

    // tlog("downscale from %ux%ux%u to %ux%u\n", width(), height(), depth(), nw, nh);
    if (!valid()) {
        tlog("not a valid YXImage\n");
        goto error;
    }
    {
        unsigned w = width();
        unsigned h = height();
        unsigned d = depth();
        Visual *v = xapp->visual();
        bool has_alpha = hasAlpha();

        ximage = XCreateImage(xapp->display(), v, d, ZPixmap, 0, NULL, nw, nh, 8, 0);
        if (!ximage || !(ximage->data = (char*) calloc(ximage->bytes_per_line*nh, sizeof(char)))) {
            tlog("could not allocate ximage %ux%ux%u or data\n", nw, nh, d);
            goto error;
        }
        size_t alloc_size = ximage->bytes_per_line * ximage->height * 4 * sizeof(*chanls);
        // tlog("created downscale ximage at %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        if (!(chanls = new double[alloc_size])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        memset(chanls, double(0), alloc_size);
        alloc_size = ximage->bytes_per_line * ximage->height * sizeof(*counts);
        if (!(counts = new double[alloc_size])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        memset(counts, double(0), alloc_size);
        if (!(colors = new double[alloc_size])) {
            tlog("could not allocate working arrays\n");
            goto error;
        }
        memset(colors, double(0), alloc_size);
        {
            double pppx = (double) w / (double) nw;
            double pppy = (double) h / (double) nh;

            double ty, by; unsigned l;
            for (ty = 0.0, by = pppy, l = 0; l < nh; l++, ty += pppy, by += pppy) {
                for (unsigned j = floor(ty); j < by; j++) {
                    double yf = 1.0;
                    if (j < ty && j + 1 > ty)
                        yf = ty - j;
                    else if (j < by && j + 1 > by)
                        yf = by - j;
                    double lx, rx; unsigned k;
                    for (lx = 0.0, rx = pppx, k = 0; k < nw; k++, lx += pppx, rx += pppx) {
                        for (unsigned i = floor(lx); i < rx; i++) {
                            double xf = 1.0;
                            if (i < lx && i + 1 > lx)
                                xf = lx - i;
                            else if (i < rx && i + 1 > rx)
                                xf = rx - i;
                            double ff = xf * yf;
                            unsigned m = l * nw + k;
                            unsigned n = m << 2;
                            counts[m] += ff;
                            unsigned long pixel = XGetPixel(fImage, i, j);
                            unsigned A = has_alpha ? (pixel >> 24) & 0xff : 255;
                            unsigned R = (pixel >> 16) & 0xff;
                            unsigned G = (pixel >>  8) & 0xff;
                            unsigned B = (pixel >>  0) & 0xff;
                            colors[m] += ff;
                            chanls[n+0] += A * ff;
                            chanls[n+1] += R * ff;
                            chanls[n+2] += G * ff;
                            chanls[n+3] += B * ff;
                        }
                    }
                }
            }
            unsigned amax = 0;
            for (unsigned l = 0; l < nh; l++) {
                for (unsigned k = 0; k < nw; k++) {
                    unsigned m = l * nw + k;
                    unsigned n = m << 2;
                    unsigned long pixel = 0;
                    if (counts[m])
                        pixel |= (lround(chanls[n+0] / counts[m]) & 0xff) << 24;
                    if (colors[m]) {
                        pixel |= (lround(chanls[n+1] / colors[m]) & 0xff) << 16;
                        pixel |= (lround(chanls[n+2] / colors[m]) & 0xff) <<  8;
                        pixel |= (lround(chanls[n+3] / colors[m]) & 0xff) <<  0;
                    }
                    XPutPixel(ximage, k, l, pixel);
                    amax = max(amax, (unsigned)((pixel >> 24) & 0xff));
                }
            }
            if (!amax)
                /* no opacity at all! */
                for (unsigned l = 0; l < nh; l++)
                    for (unsigned k = 0; k < nw; k++)
                        XPutPixel(ximage, k, l, XGetPixel(ximage, k, l) | 0xFF000000);
            else if (amax < 255) {
                double bump = (double) 255 / (double) amax;
                for (unsigned l = 0; l < nh; l++)
                    for (unsigned k = 0; k < nw; k++) {
                        unsigned long pixel = XGetPixel(ximage, k, l);
                        amax = (pixel >> 24) & 0xff;
                        amax = lround((double) amax * bump);
                        if (amax > 255)
                            amax = 255;
                        pixel = (pixel & 0x00FFFFFF) | (amax << 24);
                        XPutPixel(ximage, k, l, pixel);
                    }
            }
            image.init(new YXImage(ximage));
            ximage = 0; // consumed above
        }
    }
  error:
    if (chanls)
        delete[] chanls;
    if (counts)
        delete[] counts;
    if (colors)
        delete[] colors;
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YXImage::subimage(int x, int y, unsigned w, unsigned h)
{
    ref<YImage> image;
    XImage *ximage;

    // tlog("copy from %ux%u+%d+%d from %ux%ux%u\n", w, h, x, y, width(), height(), depth());
    if (!valid()) {
        tlog("invalid YXImage\n");
        goto error;
    }
    ximage = XSubImage(fImage, x, y, w, h);
    if (!ximage) {
        tlog("could not create subimage\n");
        goto error;
    }
    image.init(new YXImage(ximage));
  error:
    return image;
}

ref<YImage> YXImage::scale(unsigned nw, unsigned nh)
{
    ref<YImage> image;

    if (!valid()) {
        tlog("invalid YXImage\n");
        return image;
    }
    unsigned w = width();
    unsigned h = height();
    if (nw == w && nh == h)
        return subimage(0, 0, w, h);
    if (nw <= w && nh <= h)
        return downscale(nw, nh);
    return upscale(nw, nh);
}

ref<YImage> YImage::createFromPixmap(ref<YPixmap> pixmap)
{
    return createFromPixmapAndMask(pixmap->pixmap(),
                                   pixmap->mask(),
                                   pixmap->width(),
                                   pixmap->height());
}

ref<YImage> YImage::createFromPixmapAndMask(Pixmap pixmap, Pixmap mask,
                                            unsigned width,
                                            unsigned height)
{
    ref<YImage> image;
    XImage *xdraw, *xmask = 0;

    // tlog("creating YImage from pixmap 0x%lu mask 0x%lu %ux%u", pixmap, mask, width, height);
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    xdraw = XGetImage(xapp->display(), pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    if (xdraw && (!mask || (xmask = XGetImage(xapp->display(), mask, 0, 0, width, height, 0x1, XYPixmap)))) {
        // tlog("got pixmap ximage %ux%ux%u\n", xdraw->width, xdraw->height, xdraw->depth);
        // if (xmask)
        //     tlog("got mask ximage %ux%ux%u\n", xmask->width, xmask->height, xmask->depth);
        image = YXImage::combine(xdraw, xmask);
    }
    if (xdraw)
        XDestroyImage(xdraw);
    if (xmask)
        XDestroyImage(xmask);
    return image;
}

ref<YImage> YXImage::combine(XImage *xdraw, XImage *xmask)
{
    ref<YImage> image;
    XImage *ximage = 0;

    unsigned w = xdraw->width;
    unsigned h = xdraw->height;

    // tlog("combining ximage draw %ux%ux%u and mask\n", xdraw->width, xdraw->height, xdraw->depth);
    if (!xmask) {
        ximage = XSubImage(xdraw, 0, 0, w, h);
        if (!ximage)
            goto error;
        tlog("no mask, subimage %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        image.init(new YXImage(ximage));
        return image;
    }
    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, w, h, 8, 0);
    if (!ximage || !(ximage->data = (char*) calloc(ximage->bytes_per_line*h, sizeof(char))))
        goto error;
    // tlog("created ximage for combine at %ux%ux%u with mask %ux%ux%u\n",
    //      ximage->width, ximage->height, ximage->depth,
    //      xmask->width, xmask->height, xmask->depth);
    for (unsigned j = 0; j < h; j++)
        for (unsigned i = 0; i < w; i++)
            if (XGetPixel(xmask, i, j))
                XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) | 0xFF000000);
            else
                XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) & 0x00FFFFFF);
    image.init(new YXImage(ximage));
    return image;
  error:
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YImage::createFromIconProperty(long *prop_pixels, unsigned w, unsigned h)
{
    ref<YImage> image;
    XImage *ximage;

    // tlog("creating icon %ux%u\n", w, h);
    // icon properties are always 32-bit ARGB
    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, w, h, 8, 0);
    if (!ximage || !(ximage->data = (char*) calloc(ximage->bytes_per_line*h, sizeof(char))))
        goto error;
    // tlog("created ximage for icon %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
    for (unsigned j = 0; j < h; j++)
        for (unsigned i = 0; i < w; i++, prop_pixels++)
            XPutPixel(ximage, i, j, *prop_pixels);
    image.init(new YXImage(ximage));
    return image;
  error:
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YImage::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                   unsigned w, unsigned h,
                                                   unsigned nw, unsigned nh)
{
    ref<YImage> image = createFromPixmapAndMask(pix, mask, w, h);
    if (image != null)
        image = image->scale(nw, nh);
    return image;
}

ref <YPixmap> YXImage::renderToPixmap(unsigned depth)
{
	ref <YPixmap> pixmap;
	bool has_mask = false;
	XImage *xdraw = 0, *xmask = 0;
	Pixmap draw = None, mask = None;
	XGCValues xg;
	GC gcd = None, gcm = None;

	if (!valid())
		goto done;
    // tlog("rendering %ux%ux%u to pixmap\n", width(), height(), depth());
    {
        unsigned w = width();
        unsigned h = height();
        if (hasAlpha())
            for (unsigned j = 0; !has_mask && j < h; j++)
                for (unsigned i = 0; !has_mask && i < w; i++)
                    if (((XGetPixel(fImage, i, j) >> 24) & 0xff) < 128)
                        has_mask = true;
        if (hasAlpha()) {
            xdraw = XCreateImage(xapp->display(), xapp->visual(), depth, ZPixmap, 0, NULL, w, h, 8, 0);
            if (!xdraw || !(xdraw->data = (char*) calloc(xdraw->bytes_per_line*h, sizeof(char))))
                goto done;
            for (unsigned j = 0; j < h; j++)
                for (unsigned i = 0; i < w; i++)
                    XPutPixel(xdraw, i, j, XGetPixel(fImage, i, j));
        } else if (!(xdraw = XSubImage(fImage, 0, 0, w, h)))
            goto done;
        // tlog("created ximage %ux%ux%u for pixmap\n", xdraw->width, xdraw->height, xdraw->depth);

        xmask = XCreateImage(xapp->display(), xapp->visual(), 1, XYPixmap, 0, NULL, w, h, 8, 0);
        if (!xmask || !(xmask->data = (char*) calloc(xmask->bytes_per_line*h, sizeof(char))))
            goto done;
        for (unsigned j = 0; j < h; j++)
            for (unsigned i = 0; i < w; i++)
                XPutPixel(xmask, i, j,
                        has_mask ? (((XGetPixel(fImage, i, j) >> 24) & 0xff) < 128 ? 0 : 1) : 1);
        // tlog("created ximage %ux%ux%u for mask\n", xmask->width, xmask->height, xmask->depth);

        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        draw = XCreatePixmap(xapp->display(), xapp->root(), xdraw->width, xdraw->height, xdraw->depth);
        if (!draw)
            goto done;
        // tlog("created pixmap 0x%lx %ux%ux%u for pixmap\n", draw, xdraw->width, xdraw->height, xdraw->depth);
        gcd = XCreateGC(xapp->display(), draw, 0UL, &xg);
        if (!gcd)
            goto done;
        // tlog("putting ximage %ux%ux%u to pixmap\n", xdraw->width, xdraw->height, xdraw->depth);
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), draw, gcd, xdraw, 0, 0, 0, 0, xdraw->width, xdraw->height);

        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        mask = XCreatePixmap(xapp->display(), xapp->root(), xmask->width, xmask->height, xmask->depth);
        if (!mask)
            goto done;
        // tlog("created pixmap 0x%lx %ux%ux%u for mask\n", mask, xmask->width, xmask->height, xmask->depth);
        gcm = XCreateGC(xapp->display(), mask, 0UL, &xg);
        if (!gcm)
            goto done;
        // tlog("putting ximage %ux%ux%u to mask\n", xmask->width, xmask->height, xmask->depth);
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), mask, gcm, xmask, 0, 0, 0, 0, xmask->width, xmask->height);

        pixmap = createPixmap(draw, mask, xdraw->width, xdraw->height, depth);
        draw = mask = None; // consumed above
    }
  done:
	if (gcm) {
		XFreeGC(xapp->display(), gcm);
    }
	if (gcd) {
		XFreeGC(xapp->display(), gcd);
    }
    if (mask) {
        XFreePixmap(xapp->display(), mask);
    }
    if (draw) {
        XFreePixmap(xapp->display(), draw);
    }
	if (xmask) {
		XDestroyImage(xmask);
    }
	if (xdraw) {
		XDestroyImage(xdraw);
    }
	return pixmap;
}

ref<YPixmap> YImage::createPixmap(Pixmap draw, Pixmap mask,
                                  unsigned w, unsigned h, unsigned depth)
{
    return ref<YPixmap>(new YPixmap(draw, mask, w, h, depth));
}

void YXImage::draw(Graphics& g, int dx, int dy)
{
    composite(g, 0, 0, width(), height(), dx, dy);
}

void YXImage::draw(Graphics& g, int x, int y,
                   unsigned w, unsigned h, int dx, int dy)
{
    composite(g, x, y, w, h, dx, dy);
}

void YXImage::composite(Graphics& g, int x, int y,
                        unsigned w, unsigned h, int dx, int dy)
{
    XImage *xback = 0;

    if (!valid())
        return;
    unsigned wi = width();
    unsigned hi = height();
    // tlog("compositing %ux%u+%d+%d of %ux%ux%u onto drawable at +%d+%d\n",
    //         w, h, x, y, width(), height(), depth(), dx, dy);
    if (g.xorigin() > dx) {
        if ((int) w <= g.xorigin() - dx)
            return;
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if ((int) h <= g.xorigin() - dx)
            return;
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if ((int) (dx + w) > (int) (g.xorigin() + g.rwidth())) {
        if ((int) (g.xorigin() + g.rwidth()) <= dx)
            return;
        w = g.xorigin() + g.rwidth() - dx;
    }
    if ((int) (dy + h) > (int) (g.yorigin() + g.rheight())) {
        if ((int) (g.yorigin() + g.rheight()) <= dy)
            return;
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0)
        return;

    if (!hasAlpha()) {
        // tlog("simply putting %ux%u+0+0 of ximage %ux%ux%u onto drawable at +%d+%d\n",
        //         w, h, width(), height(), depth(), dx-g.xorigin(), dy-g.yorigin());
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), g.drawable(), g.handleX(), fImage,  0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h);
        return;
    }
    // tlog("getting image %ux%u+%d+%d from drawable\n", w, h, dx-g.xorigin(), dy-g.yorigin());
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    xback = XGetImage(xapp->display(), g.drawable(), dx - g.xorigin(), dy - g.yorigin(), w, h, AllPlanes, ZPixmap);
    if (!xback)
        return;
    // tlog("got ximage %ux%ux%u\n", xback->width, xback->height, xback->depth);

    // tlog("compositing %ux%u+%d+%d of %ux%ux%u onto %ux%ux%u\n",
    //         w, h, x, y, width(), height(), depth(), xback->width, xback->height, xback->depth);
    for (unsigned j = 0; j < h; j++) {
        if ((int)j + y < 0 || (int)j + y > (int)hi)
            continue;
        for (unsigned i = 0; i < w; i++) {
            if ((int)i + x < 0 || (int)i + x > (int)wi)
                continue;
            unsigned Rb, Gb, Bb, A, R, G, B;
            unsigned long pixel;

            pixel = XGetPixel(xback, i, j);
            Rb = (pixel >> 16) & 0xff;
            Gb = (pixel >>  8) & 0xff;
            Bb = (pixel >>  0) & 0xff;
            pixel = XGetPixel(fImage, i + x, j + y);
            A = (pixel >> 24) & 0xff;
            R = (pixel >> 16) & 0xff;
            G = (pixel >>  8) & 0xff;
            B = (pixel >>  0) & 0xff;

            Rb = ((A * R) + ((255 - A) * Rb)) / 255;
            Gb = ((A * G) + ((255 - A) * Gb)) / 255;
            Bb = ((A * B) + ((255 - A) * Bb)) / 255;
            pixel = (Rb << 16)|(Gb << 8)|(Bb << 0);
            XPutPixel(xback, i, j, pixel);
        }
    }
    // tlog("putting %ux%u+0+0 of ximage %ux%ux%u on drawable at +%d+%d\n",
    //         w, h, xback->width, xback->height, xback->depth, dx-g.xorigin(), dy-g.yorigin());
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    XPutImage(xapp->display(), g.drawable(), g.handleX(), xback, 0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h);
    XDestroyImage(xback);
}

void image_init()
{
}

#endif


// vim: set sw=4 ts=4 et:
