#include "config.h"

#ifdef CONFIG_IMLIB2

#include "yimage2.h"
#include "yxapp.h"

unsigned YImage2::instances;
GC YImage2::gcs[3];

GC YImage2::gc(Drawable draw, unsigned depth) {
    int i;
    switch (depth) {
        case 1 : i = 0; break;
        case 24: i = 1; break;
        case 32: i = 2; break;
        default: return None;
    }
    if (gcs[i] == None)
        gcs[i] = XCreateGC(xapp->display(), draw, None, None);
    return gcs[i];
}

void YImage2::freegcs() {
    for (int i = 0; i < 3; ++i)
        if (gcs[i]) {
            if (xapp)
                XFreeGC(xapp->display(), gcs[i]);
            gcs[i] = None;
        }
}

const char* YImage::renderName() {
    return "Imlib2";
}

bool YImage::supportsDepth(unsigned depth) {
    return depth == xapp->depth() || depth == 32;
}

bool YImage2::hasAlpha() const {
    context();
    return imlib_image_has_alpha();
}

unsigned YImage2::depth() const {
    context();
    return imlib_image_has_alpha() ? 32 : 24;
}

ref<YImage> YImage::load(upath filename) {
    Image image = imlib_load_image_immediately_without_cache(filename.string());
    if (image) {
        imlib_context_set_image(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        int w = imlib_image_get_width();
        int h = imlib_image_get_height();
        DATA32* data = imlib_image_get_data();
        DATA32* stop = data + w * h;
        if (imlib_image_has_alpha()) {
            for (DATA32* p = data; p < stop; ++p) {
                if ((*p >> 24) < ATH) {
                    *p = 0;
                }
            }
        } else {
            for (DATA32* p = data; p < stop; ++p) {
                *p |= 0xFF000000;
            }
            imlib_image_set_has_alpha(1);
        }
        imlib_image_put_back_data(data);
        return ref<YImage>(new YImage2(w, h, image));
    }

    // support themes with indirect XPM images, like OnyX:
    const int lim = 64;
    for (int k = 9; --k > 0 && inrange(int(filename.fileSize()), 5, lim); ) {
        fileptr fp(filename.fopen("r"));
        if (fp == nullptr)
            break;

        char buf[lim];
        if (fgets(buf, lim, fp) == nullptr)
            break;

        mstring match(mstring(buf).match("^[a-z][-_a-z0-9]*\\.xpm$", "i"));
        if (match == null)
            break;

        filename = filename.parent().relative(match);
        if (filename.fileSize() > lim)
            return load(filename);
    }
    return null;
}

void YImage2::save(upath filename) {
    context();
    imlib_image_set_format("png");
    imlib_save_image(filename.replaceExtension(".png").string());
}

ref<YImage2> YImage2::twoHigh(unsigned h) {
    context();
    unsigned char* top = (unsigned char *) imlib_image_get_data();
    unsigned char* bot = top + (4 * width());
    Image image = imlib_create_image(int(width()), int(h));
    if (image) {
        context(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        imlib_image_set_has_alpha(1);
        unsigned char* dst = (unsigned char *) imlib_image_get_data();
        for (unsigned i = 0; i < width(); ++i) {
            for (int j = 0; j < 4; ++j) {
                unsigned char* ptr = dst + (4 * i) + j;
                unsigned t = *top++;
                unsigned b = *bot++;
                for (unsigned k = 0; k < h; ++k) {
                    *ptr = (t * (h - 1 - k) + b * k) / (h - 1);
                    ptr += 4 * width();
                }
            }
        }
        imlib_image_put_back_data((DATA32 *) dst);
        return ref<YImage2>(new YImage2(width(), h, image));
    }
    return null;
}

ref<YImage2> YImage2::twoWide(unsigned w) {
    context();
    unsigned char* src = (unsigned char *) imlib_image_get_data();
    Image image = imlib_create_image(int(w), int(height()));
    if (image) {
        context(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        imlib_image_set_has_alpha(1);
        unsigned char* dst = (unsigned char *) imlib_image_get_data();
        for (unsigned i = 0; i < height(); ++i) {
            for (unsigned k = 0; k < w; ++k) {
                unsigned char* ptr = dst + (4 * (i * w + k));
                for (int j = 0; j < 4; ++j) {
                    unsigned l = src[j];
                    unsigned r = src[j + 4];
                    *ptr++ = (l * (w - 1 - k) + r * k) / (w - 1);
                }
            }
            src += 8;
        }
        imlib_image_put_back_data((DATA32 *) dst);
        return ref<YImage2>(new YImage2(w, height(), image));
    }
    return null;
}

ref<YImage> YImage2::scale(unsigned w, unsigned h) {
    if (w == width() && h == height())
        return ref<YImage>(this);

    ref<YImage2> im(this);
    if (height() == 2 && 2 < h) {
        im = im->twoHigh(h);
        if (im == null)
            return null;
    }
    if (width() == 2 && 2 < w) {
        im = im->twoWide(w);
        if (im == null)
            return null;
    }
    if (w == im->width() && h == im->height())
        return ref<YImage>(im);

    im->context();
    imlib_context_set_anti_alias(1);
    Image image = imlib_create_cropped_scaled_image(0, 0,
                   int(im->width()), int(im->height()), int(w), int(h));
    if (image) {
        imlib_context_set_image(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        return ref<YImage>(new YImage2(w, h, image));
    }
    return null;
}

ref<YImage> YImage2::subimage(int x, int y, unsigned w, unsigned h) {
    PRECONDITION(w <= width() && unsigned(x) <= width() - w);
    PRECONDITION(h <= height() && unsigned(y) <= height() - h);

    context();
    Image image = imlib_create_cropped_image(x, y, int(w), int(h));
    if (image) {
        imlib_context_set_image(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        return ref<YImage>(new YImage2(w, h, image));
    }
    return null;
}

ref<YImage> YImage::createFromPixmap(ref<YPixmap> pixmap) {
    return createFromPixmapAndMask(pixmap->pixmap(),
                                   pixmap->mask(),
                                   pixmap->width(),
                                   pixmap->height());
}

ref<YImage> YImage::createFromPixmapAndMask(Pixmap pixmap, Pixmap mask,
                                            unsigned width, unsigned height)
{
    imlib_context_set_drawable(pixmap);
    imlib_context_set_mask(mask);
    Image image = imlib_create_image_from_drawable(mask, 0, 0,
                                   int(width), int(height), 0);
    imlib_context_set_drawable(None);
    imlib_context_set_mask(None);
    if (image) {
        imlib_context_set_image(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        return ref<YImage>(new YImage2(width, height, image));
    }
    return null;
}

ref<YImage> YImage::createFromIconProperty(long* prop_pixels,
                                           unsigned width, unsigned height)
{
    Image image = imlib_create_image(int(width), int(height));
    if (image) {
        imlib_context_set_image(image);
        imlib_context_set_mask_alpha_threshold(ATH);
        imlib_image_set_has_alpha(1);
        DATA32* data = imlib_image_get_data();
        DATA32* stop = data + width * height;
        long* p = prop_pixels;
        const DATA32 limit = ATH << 24;
        unsigned alps = 0;
        for (DATA32* d = data; d < stop; d++, p++) {
            *d = (DATA32) *p;
            alps += (*d >= limit);
        }
        if (alps && alps >= (width + height) / alps) {
            for (DATA32* d = data; d < stop; d++) {
                if (*d < limit) {
                    *d = 0;
                }
            }
        } else {
            for (DATA32* d = data; d < stop; d++) {
                *d |= 0xFF000000;
            }
        }
        imlib_image_put_back_data(data);
        return ref<YImage>(new YImage2(width, height, image));
    }
    return null;
}

ref<YImage> YImage::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                  unsigned width, unsigned height,
                                                  unsigned nw, unsigned nh)
{
    ref<YImage> image = createFromPixmapAndMask(pix, mask, width, height);
    if (image != null && (nw != width || nh != height))
        image = image->scale(nw, nh);
    return image;
}

ref<YPixmap> YImage2::renderToPixmap(unsigned depth, bool premult) {
    Pixmap pixmap = None, mask = None;

    context();
    if (depth == 0) {
        depth = xapp->depth();
    }
    if (depth == 32 || depth == 24) {
        int width = int(this->width());
        int height = int(this->height());
        Visual* visual = xapp->visualForDepth(depth);
        XImage* image = XCreateImage(xapp->display(), visual, depth,
                                     ZPixmap, 0, nullptr, width, height, 8, 0);
        if (image)
            image->data = (char *) calloc(image->bytes_per_line * height, 1);
        XImage* imask = XCreateImage(xapp->display(), visual, 1,
                                     XYPixmap, 0, nullptr, width, height, 8, 0);
        if (imask)
            imask->data = (char *) calloc(imask->bytes_per_line * height, 1);

        if (image && image->data && imask && imask->data) {
            const bool alpha = imlib_image_has_alpha();
            DATA32* data = imlib_image_get_data_for_reading_only();

            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    DATA32 pixel = data[col + row * width];
                    unsigned char red = ((pixel >> 16) & 0xFF);
                    unsigned char grn = ((pixel >> 8) & 0xFF);
                    unsigned char blu = (pixel & 0xFF);
                    unsigned char alp = !alpha ? 0x00 :
                        (pixel < (ATH << 24)) ? 0x00 : ((pixel >> 24) & 0xFF);
                    if (premult) {
                        red = (red * (alp + 1)) >> 8;
                        grn = (grn * (alp + 1)) >> 8;
                        blu = (blu * (alp + 1)) >> 8;
                    }
                    XPutPixel(image, col, row,
                              (red << 16) |
                              (grn << 8) |
                              (blu << 0) |
                              (alp << 24));
                    bool bit = (alp >= ATH);
                    XPutPixel(imask, col, row, bit);
                }
            }

            pixmap = XCreatePixmap(xapp->display(), xapp->root(),
                                   width, height, depth);
            XPutImage(xapp->display(), pixmap, gc(pixmap, depth), image,
                      0, 0, 0, 0, width, height);

            mask = XCreatePixmap(xapp->display(), xapp->root(),
                                 width, height, 1);
            XPutImage(xapp->display(), mask, gc(mask, 1), imask,
                      0, 0, 0, 0, width, height);

            XDestroyImage(image);
            XDestroyImage(imask);
        }
    }
    else {
        pixmap = XCreatePixmap(xapp->display(), xapp->root(),
                               width(), height(), depth);
        if (pixmap) {
            mask = XCreatePixmap(xapp->display(), xapp->root(),
                                 width(), height(), 1);
            imlib_context_set_drawable(pixmap);
            imlib_context_set_mask(mask);
            imlib_render_image_on_drawable(0, 0);
            imlib_context_set_drawable(None);
            imlib_context_set_mask(None);
        }
    }

    if (pixmap) {
        return createPixmap(pixmap, mask, width(), height(), depth);
    } else {
        return null;
    }
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask,
                                  unsigned w, unsigned h, unsigned depth) {
    ref<YPixmap> n;

    if (pixmap)
        n.init(new YPixmap(pixmap, mask, w, h, depth, ref<YImage>(this)));
    return n;
}

void YImage2::draw(Graphics& g, int dx, int dy) {
    composite(g, 0, 0, width(), height(), dx, dy);
}

void YImage2::draw(Graphics& g, int x, int y, unsigned w, unsigned h, int dx, int dy) {
    composite(g, x, y, w, h, dx, dy);
}

void YImage2::composite(Graphics& g, int x, int y, unsigned width, unsigned height, int dx, int dy) {
    int w = int(width);
    int h = int(height);

    if (g.xorigin() > dx) {
        if (w <= g.xorigin() - dx)
            return;
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if (h <= g.yorigin() - dy)
            return;
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if (dx + w > int(g.xorigin() + g.rwidth())) {
        if (int(g.xorigin() + g.rwidth()) <= dx)
            return;
        w = g.xorigin() + g.rwidth() - dx;
    }
    if (dy + h > int(g.yorigin() + g.rheight())) {
        if (int(g.yorigin() + g.rheight()) <= dy)
            return;
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0)
        return;

    context();
    imlib_context_set_drawable(g.drawable());
    imlib_context_set_mask_alpha_threshold(ATH);
    imlib_context_set_blend(1);
    imlib_render_image_part_on_drawable_at_size(x, y, w, h, dx, dy, w, h);
    imlib_context_set_drawable(None);
    imlib_context_set_blend(0);
}

void YImage2::copy(Graphics& g, int x, int y) {
    context();
    imlib_context_set_mask_alpha_threshold(ATH);
    imlib_context_set_drawable(g.drawable());
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(x, y);
}

void image_init() {
    imlib_set_cache_size(0);
    imlib_context_set_display(xapp->display());
    imlib_context_set_visual(xapp->visual());
    imlib_context_set_colormap(xapp->colormap());
    imlib_context_set_anti_alias(1);
    imlib_context_set_mask_alpha_threshold(ATH);
}

#endif

// vim: set sw=4 ts=4 et:
