#include "config.h"

#if defined CONFIG_XPM

#include <math.h>
#include <errno.h>
#include "yimage.h"
#include "yxapp.h"
#include "intl.h"

#include <X11/xpm.h>

#ifdef CONFIG_LIBPNG
#include <png.h>
#endif
#ifdef CONFIG_LIBJPEG
#include <jpeglib.h>
#include <setjmp.h>
#endif

class YXImage: public YImage {
public:
    YXImage(XImage *ximage, bool bitmap = false) : YImage(ximage->width, ximage->height), fImage(ximage), fBitmap(bitmap) {
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
#ifdef CONFIG_LIBJPEG
    static ref<YImage> loadjpg(upath filename);
#endif
    static ref<YImage> combine(XImage *xdraw, XImage *xmask);

private:
    bool isBitmap() const { return fBitmap; }
    bool hasAlpha() const { return fImage ? fImage->depth == 32 : false; }
    virtual ref<YImage> upscale(unsigned width, unsigned height);
    virtual ref<YImage> downscale(unsigned width, unsigned height);
    virtual ref<YImage> subimage(int x, int y, unsigned width, unsigned height);
    XImage *fImage;
    bool fBitmap;
};

ref<YImage> YImage::load(upath filename)
{
    ref<YImage> image;
    pstring ext(filename.getExtension().lower());
    bool unsup = false;

    if (ext == ".xbm")
        image = YXImage::loadxbm(filename);
    else if (ext == ".xpm")
        image = YXImage::loadxpm(filename);
    else if (ext == ".png") {
#ifdef CONFIG_LIBPNG
        image = YXImage::loadpng(filename);
#else
        { static int count; if (count < 1) { ++count;
            warn(_("Support for PNG images was not enabled")); } }
#endif
    }
    else if (ext == ".jpg" || ext == ".jpeg") {
#ifdef CONFIG_LIBJPEG
        image = YXImage::loadjpg(filename);
#else
        { static int count; if (count < 1) { ++count;
            warn(_("Support for JPEG images was not enabled")); } }
#endif
    }
    else {
        unsup = true;
        static int count; if (count < 1) { ++count;
        warn(_("Unsupported file format: %s"), cstring(ext).c_str()); }
    }

    if (image == null && !unsup)
            warn(_("Out of memory for pixmap \"%s\""), filename.string().c_str());
    return image;
}

ref <YImage> YXImage::loadxbm(upath filename)
{
	ref < YImage > image;
	XImage *xdraw = 0;
	unsigned w, h;
	int x, y;
	char *data = 0;
	int status;

	status = XReadBitmapFileData(filename.string(), &w, &h, (unsigned char **) &data, &x, &y);
	if (status != BitmapSuccess) {
        tlog("ERROR: could not read pixmap file %s\n", filename.string().c_str());
		goto error;
    }
	xdraw = XCreateImage(xapp->display(), None, 1, XYBitmap, 0, NULL, w, h, 8, 0);
	if (!xdraw) {
        tlog("ERROR: could not create bitmap image\n");
		goto error;
    }
	xdraw->data = data;
	data = 0;
	image = YXImage::combine(xdraw, xdraw);
  error:
	if (xdraw)
		XDestroyImage(xdraw);
	if (data)
		free(data);
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
            if (ximage && (ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line*h, sizeof(char)))) {
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
    volatile void *vol_png_pixels, *vol_row_pointers;
    volatile void *vol_ximage;
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
    png_pixels = (typeof(png_pixels))calloc(row_bytes * height, sizeof(*png_pixels));
    vol_png_pixels = png_pixels;
    row_pointers = (typeof(row_pointers))calloc(height, sizeof(*row_pointers));
    vol_row_pointers = row_pointers;
    for (i = 0; i < height; i++)
        row_pointers[i] = png_pixels + i * row_bytes;
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, info_ptr);
    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, width, height, 8, 0);
    if (!ximage || !(ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, height))) {
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
    free(png_pixels);
    row_pointers = (png_byte **) vol_row_pointers;
    free(row_pointers);
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

#ifdef CONFIG_LIBJPEG
struct jpeg_error_jmp : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

static void my_jpeg_error_exit(j_common_ptr cinfo) {
    jpeg_error_jmp* myjmp = (jpeg_error_jmp*) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myjmp->setjmp_buffer, 1);
}

ref<YImage> YXImage::loadjpg(upath filename)
{
    ref<YImage> yximage;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_jmp jerr;
    JSAMPARRAY buffer;
    int row_stride;
    FILE* infile = filename.fopen("rb");
    if (infile == 0) {
        fail("could not open %s", filename.string().c_str());
        return yximage;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_jpeg_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return yximage;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    int width = int(cinfo.output_width);
    int height = int(cinfo.output_height);
    XImage* ximage = XCreateImage(xapp->display(), xapp->visual(),
                                  32, ZPixmap, 0, NULL,
                                  width, height, 8, 0);
    if (ximage)
        ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, height);
    if (ximage && ximage->data) {
        for (int line; (line = int(cinfo.output_scanline)) < height; ) {
            (void) jpeg_read_scanlines(&cinfo, buffer, 1);
            unsigned char* buf = (unsigned char *) buffer[0];
            for (int i = 0; i < width; ++i, buf += 3) {
                XPutPixel(ximage, i, line,
                          (buf[0] << 16) |
                          (buf[1] <<  8) |
                          (buf[2]      ) |
                          0xFF000000);
            }
        }
        yximage.init(new YXImage(ximage));
    }
    else if (ximage)
        XDestroyImage(ximage);

    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return yximage;
}
#endif

ref<YImage> YXImage::upscale(unsigned nw, unsigned nh)
{
    ref<YImage> image;
    XImage *ximage = 0;
    double *chanls = 0;
    double *counts = 0;
    double *colors = 0;

    if (!valid()) {
        tlog("ERROR: not a valid YXImage\n");
        goto error;
    }
    {
        unsigned w = fImage->width;
        unsigned h = fImage->height;
        unsigned d = fImage->depth;
        Visual *v = xapp->visual();
        bool has_alpha = hasAlpha();

        // tlog("upscale from %ux%ux%u to %ux%u\n", w, h, d, nw, nh);
        ximage = XCreateImage(xapp->display(), v, d, ZPixmap, 0, NULL, nw, nh, 8, 0);
        if (!ximage || !(ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, ximage->height))) {
            tlog("ERROR: could not allocate ximage %ux%ux%u or data\n", nw, nh, d);
            goto error;
        }
        // tlog("created upscale ximage at %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        if (!(chanls = (typeof(chanls))calloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(counts = (typeof(counts))calloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(colors = (typeof(colors))calloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
            tlog("ERROR: could not allocate working arrays\n");
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
                            if (fBitmap && (pixel & 0x00FFFFFF))
                                pixel |= 0x00FFFFFF;
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
            image.init(new YXImage(ximage, fBitmap));
            ximage = 0; // consumed above
        }
    }
  error:
    if (chanls)
        free(chanls);
    if (counts)
        free(counts);
    if (colors)
        free(colors);
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

    if (!valid()) {
        tlog("ERROR: not a valid YXImage\n");
        goto error;
    }
    {
        unsigned w = fImage->width;
        unsigned h = fImage->height;
        unsigned d = fImage->depth;
        Visual *v = xapp->visual();
        bool has_alpha = hasAlpha();

        // tlog("downscale from %ux%ux%u to %ux%u\n", w, h, d, nw, nh);
        ximage = XCreateImage(xapp->display(), v, d, ZPixmap, 0, NULL, nw, nh, 8, 0);
        if (!ximage || !(ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, ximage->height))) {
            tlog("ERROR: could not allocate ximage %ux%ux%u or data\n", nw, nh, d);
            goto error;
        }
        // tlog("created downscale ximage at %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        if (!(chanls = (typeof(chanls))calloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(counts = (typeof(counts))calloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(colors = (typeof(colors))calloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        {
            double pppx = (double) nw / (double) w;
            double pppy = (double) nh / (double) h;

            double lx, rx, ty, by, xf, yf, ff;

            unsigned long pixel;
            unsigned i, j, k, l, m, n, A, R, G, B;

            for (ty = 0.0, by = pppy, l = 0; l < h; l++, ty = by, by = (l + 1) *pppy) {
                for (j = floor(ty); j < by; j++) {

                    if (ty < (j + 1) && (j + 1) < by) yf = (j + 1) - ty;
                    else if (ty < j && j < by) yf = by - j;
                    else yf = 1.0;

                    for (lx = 0.0, rx = pppx, k = 0; k < w; k++, lx = rx, rx = (k + 1) * pppx) {
                        for (i = floor(lx); i < rx; i++) {

                            if (lx < (i + 1) && (i + 1) < rx) yf = (i + 1) - lx;
                            else if (lx < i && i < rx) yf = rx - i;
                            else xf = 1.0;

                            ff = xf * yf;
                            m = j * nw + i;
                            n = m << 2;
                            counts[m] += ff;
                            pixel = XGetPixel(fImage, k, l);
                            if (fBitmap && (pixel & 0x00FFFFFF))
                                pixel |= 0x00FFFFFF;
                            A = has_alpha ? (pixel >> 24) & 0xff : 255;
                            R = (pixel >> 16) & 0xff;
                            G = (pixel >>  8) & 0xff;
                            B = (pixel >>  0) & 0xff;
                            colors[m] += ff;
                            chanls[n+0] += A * ff;
                            chanls[n+1] += R * ff;
                            chanls[n+2] += G * ff;
                            chanls[n+3] += B * ff;
                        }
                    }
                }
            }
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
            unsigned amax = 0;
            for (j = 0; j < nh; j++) {
                for (i = 0; i < nw; i++) {
                    n = j * nw + i;
                    m = n << 2;
                    pixel = 0;
                    if (counts[m])
                        pixel |= (min(255, lround(chanls[m+0] / counts[n])) & 0xff) << 24;
                    if (colors[m]) {
                        pixel |= (min(255, lround(chanls[m+1] / colors[n])) & 0xff) << 16;
                        pixel |= (min(255, lround(chanls[m+2] / colors[n])) & 0xff) <<  8;
                        pixel |= (min(255, lround(chanls[m+3] / colors[n])) & 0xff) <<  0;
                    }
                    XPutPixel(ximage, i, j, pixel);
                    amax = max(amax, ((pixel >> 24) & 0xff));
                }
            }
            if (!amax) {
                /* no opacity at all! */
                for (j = 0; j < nh; j++)
                    for (i = 0; i < nw; i++)
                        XPutPixel(ximage, i, j, XGetPixel(ximage, i, j) | 0xff000000);
            } else if (amax < 255) {
                double bump = (double) 255 / (double) amax;
                for (j = 0; j < nh; j++)
                    for (i = 0; i < nw; i++) {
                        pixel = XGetPixel(ximage, i, j);
                        amax = (pixel >> 24) & 0xff;
                        amax = min(255, lround(amax * bump));
                        pixel = (pixel & 0x00ffffff) | (amax << 24);
                        XPutPixel(ximage, i, j, pixel);
                    }
            }
            image.init(new YXImage(ximage, fBitmap));
            ximage = 0; // consumed above
        }
    }
  error:
    if (chanls)
        free(chanls);
    if (counts)
        free(counts);
    if (colors)
        free(colors);
    if (ximage)
        XDestroyImage(ximage);
    return image;
}

ref<YImage> YXImage::subimage(int x, int y, unsigned w, unsigned h)
{
    ref<YImage> image;
    XImage *ximage;

    if (!valid()) {
        tlog("ERROR: invalid YXImage\n");
        goto error;
    }
    // tlog("copy from %ux%u+%d+%d from %ux%ux%u\n", w, h, x, y, fImage->width, fImage->height, fImage->depth);
    ximage = XSubImage(fImage, x, y, w, h);
    if (!ximage) {
        tlog("ERROR: could not create subimage\n");
        goto error;
    }
    image.init(new YXImage(ximage, fBitmap));
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
    unsigned w = fImage->width;
    unsigned h = fImage->height;
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
    Window root;
    int x, y;
    unsigned w, h, b, d;

    if (!XGetGeometry(xapp->display(), pixmap, &root, &x, &y, &w, &h, &b, &d)) {
        tlog("could not get gometry of pixmap 0x%lx\n", pixmap);
        return image;
    }
    tlog("creating YXImage from pixmap 0x%lx mask 0x%lx, %ux%ux%u+%d+%d\n", pixmap, mask, w, h, d, x, y);
    if (width != w || height != h) {
        tlog("pixmap 0x%lx: width=%u, w=%u, height=%u, h=%u\n", pixmap, width, w, height, h);
    }
    // tlog("creating YImage from pixmap 0x%lu mask 0x%lu %ux%u", pixmap, mask, width, height);
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    if (d == 1)
	    xdraw = XGetImage(xapp->display(), pixmap, 0, 0, w, h, 0x1, XYPixmap);
    else
        xdraw = XGetImage(xapp->display(), pixmap, 0, 0, w, h, AllPlanes, ZPixmap);

    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    if (xdraw && (!mask || (xmask = XGetImage(xapp->display(), mask, 0, 0, w, h, 0x1, XYPixmap)))) {
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
    bool bitmap = (xdraw->depth == 1) ? true : false;

    // tlog("combining ximage draw %ux%ux%u and mask\n", xdraw->width, xdraw->height, xdraw->depth);
    if (bitmap && !xmask)
        xmask = xdraw;
    if (!xmask) {
        ximage = XSubImage(xdraw, 0, 0, w, h);
        if (!ximage) {
            tlog("ERROR: could not create subimage\n");
            goto error;
        }
        // tlog("no mask, subimage %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        image.init(new YXImage(ximage, bitmap));
        return image;
    }
    ximage = XCreateImage(xapp->display(), xapp->visual(), 32, ZPixmap, 0, NULL, w, h, 8, 0);
    if (!ximage || !(ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, ximage->height))) {
        tlog("ERROR: could not create ximage or allocate data\n");
        goto error;
    }
    // tlog("created ximage for combine at %ux%ux%u with mask %ux%ux%u\n",
    //      ximage->width, ximage->height, ximage->depth,
    //      xmask->width, xmask->height, xmask->depth);
    for (unsigned j = 0; j < h; j++)
        for (unsigned i = 0; i < w; i++)
            if (XGetPixel(xmask, i, j))
                XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) | 0xFF000000);
            else
                XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) & 0x00FFFFFF);
    image.init(new YXImage(ximage, bitmap));
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
    if (!ximage || !(ximage->data = (typeof(ximage->data))calloc(ximage->bytes_per_line, ximage->height))) {
        tlog("ERROR: could not create image %ux%ux32 or allocate memory\n", w, h);
        goto error;
    }
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

	if (!valid()) {
        tlog("ERROR: invalid YXImage\n");
		goto error;
    }
    // tlog("rendering %ux%ux%u to pixmap\n", fImage->width, fImage->height, fImage->depth);
    {
        unsigned w = fImage->width;
        unsigned h = fImage->height;
        if (hasAlpha())
            for (unsigned j = 0; !has_mask && j < h; j++)
                for (unsigned i = 0; !has_mask && i < w; i++)
                    if (((XGetPixel(fImage, i, j) >> 24) & 0xff) < 128)
                        has_mask = true;
        if (hasAlpha()) {
            xdraw = XCreateImage(xapp->display(), xapp->visual(), xapp->depth(), ZPixmap, 0, NULL, w, h, 8, 0);
            if (!xdraw || !(xdraw->data = (typeof(xdraw->data))calloc(xdraw->bytes_per_line, xdraw->height))) {
                tlog("ERROR: could not create XImage or allocate %ux%ux%u data\n", w, h, xapp->depth());
                goto error;
            }
            for (unsigned j = 0; j < h; j++)
                for (unsigned i = 0; i < w; i++)
                    XPutPixel(xdraw, i, j, XGetPixel(fImage, i, j));
        } else if (!(xdraw = XSubImage(fImage, 0, 0, w, h))) {
            tlog("ERROR: could not create subimage %ux%u\n", w, h);
            goto error;
        }
        // tlog("created ximage %ux%ux%u for pixmap\n", xdraw->width, xdraw->height, xdraw->depth);

        xmask = XCreateImage(xapp->display(), xapp->visual(), 1, XYBitmap, 0, NULL, w, h, 8, 0);
        if (!xmask || !(xmask->data = (typeof(xmask->data))calloc(xmask->bytes_per_line, xmask->height))) {
            tlog("ERROR: could not create XImage mask %ux%u\n", w, h);
            goto error;
        }
        for (unsigned j = 0; j < h; j++)
            for (unsigned i = 0; i < w; i++)
                XPutPixel(xmask, i, j,
                        has_mask ? (((XGetPixel(fImage, i, j) >> 24) & 0xff) < 128 ? 0 : 1) : 1);
        // tlog("created ximage %ux%ux%u for mask\n", xmask->width, xmask->height, xmask->depth);

        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        draw = XCreatePixmap(xapp->display(), xapp->root(), xdraw->width, xdraw->height, xdraw->depth);
        if (!draw) {
            tlog("ERROR: could not create pixmap %ux%ux%u\n", xdraw->width, xdraw->height, xdraw->depth);
            goto error;
        }
        // tlog("created pixmap 0x%lx %ux%ux%u for pixmap\n", draw, xdraw->width, xdraw->height, xdraw->depth);
        gcd = XCreateGC(xapp->display(), draw, 0UL, &xg);
        if (!gcd) {
            tlog("ERROR: could not create GC for pixmap 0x%lx %ux%ux%u\n",
                    draw, xdraw->width, xdraw->height, xdraw->depth);
            goto error;
        }
        // tlog("putting ximage %ux%ux%u to pixmap\n", xdraw->width, xdraw->height, xdraw->depth);
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), draw, gcd, xdraw, 0, 0, 0, 0, xdraw->width, xdraw->height);

        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        mask = XCreatePixmap(xapp->display(), xapp->root(), xmask->width, xmask->height, 1);
        if (!mask) {
            tlog("ERROR: could not create mask %ux%ux%u\n", xmask->width, xmask->height, 1U);
            goto error;
        }
        // tlog("created pixmap 0x%lx %ux%ux%u for mask\n", mask, xmask->width, xmask->height, xmask->depth);
        gcm = XCreateGC(xapp->display(), mask, 0UL, &xg);
        if (!gcm) {
            tlog("ERROR: could not create GC for mask 0x%lx %ux%ux1\n", mask, xmask->width, xmask->height);
            goto error;
        }
        XSetForeground(xapp->display(), gcm, 0xffffffff);
        XSetBackground(xapp->display(), gcm, 0x00000000);
        // tlog("putting ximage %ux%ux%u to mask\n", xmask->width, xmask->height, xmask->depth);
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), mask, gcm, xmask, 0, 0, 0, 0, xmask->width, xmask->height);

        pixmap = createPixmap(draw, mask, xdraw->width, xdraw->height, depth);
        draw = mask = None; // consumed above
    }
  error:
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
    return ref<YPixmap>(new YPixmap(draw, mask, w, h, depth, ref<YImage>(this)));
}

void YXImage::draw(Graphics& g, int dx, int dy)
{
    if (!valid())
        return;
    composite(g, 0, 0, fImage->width, fImage->height, dx, dy);
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
    unsigned wi = fImage->width;
    unsigned hi = fImage->height;
    unsigned di = fImage->depth;
    bool bitmap = isBitmap();
    unsigned long fg = g.color()->pixel() & 0x00FFFFFF;
    unsigned long bg = 0x00000000; /* for now */
    tlog("compositing %ux%u+%d+%d of %ux%ux%u onto drawable 0x%lx at +%d+%d\n", w, h, x, y, wi, hi, di, g.drawable(), dx, dy);
    Window root;
    int _x, _y;
    unsigned _w, _h, _b, _d;

    if (XGetGeometry(xapp->display(), g.drawable(), &root, &_x, &_y, &_w, &_h, &_b, &_d))
        tlog("drawable 0x%lx has geometry %ux%ux%u+%d+%d\n", g.drawable(), _w, _h, _d, _x, _y);
    if (g.xorigin() > dx) {
        if ((int) w <= g.xorigin() - dx) {
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if ((int) h <= g.xorigin() - dx) {
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if ((int) (dx + w) > (int) (g.xorigin() + g.rwidth())) {
        if ((int) (g.xorigin() + g.rwidth()) <= dx) {
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        w = g.xorigin() + g.rwidth() - dx;
    }
    if ((int) (dy + h) > (int) (g.yorigin() + g.rheight())) {
        if ((int) (g.yorigin() + g.rheight()) <= dy) {
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0) {
        tlog("ERROR: coordinates out of bounds\n");
        return;
    }
    if (!hasAlpha()) {
        tlog("simply putting %ux%u+0+0 of ximage %ux%ux%u onto drawable %ux%ux%u at +%d+%d\n",
              w, h, wi, hi, di, _w, _h, _d, dx-g.xorigin(), dy-g.yorigin());
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), g.drawable(), g.handleX(), fImage,  0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h);
        return;
    }
    tlog("getting image %ux%u+%d+%d from drawable %ux%ux%u\n", w, h, dx-g.xorigin(), dy-g.yorigin(), _w, _h, _d);
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    xback = XGetImage(xapp->display(), g.drawable(), dx - g.xorigin(), dy - g.yorigin(), w, h, AllPlanes, ZPixmap);
    if (!xback) {
        tlog("ERROR: could not get backing image\n");
        return;
    }
    // tlog("got ximage %ux%ux%u\n", xback->width, xback->height, xback->depth);

    // tlog("compositing %ux%u+%d+%d of %ux%ux%u onto %ux%ux%u\n",
    //         w, h, x, y, fImage->width, fImage->height, fImage->depth, xback->width, xback->height, xback->depth);
    for (unsigned j = 0; j < h; j++) {
        if ((int)j + y < 0 || (int)j + y > (int)hi) {
            tlog("ERROR: point y = %u is out of bounds\n", j);
            continue;
        }
        for (unsigned i = 0; i < w; i++) {
            if ((int)i + x < 0 || (int)i + x > (int)wi) {
                tlog("ERROR: point x = %u is out of bounds\n", i);
                continue;
            }
            unsigned Rb, Gb, Bb, A, R, G, B;
            unsigned long pixel;

            pixel = XGetPixel(xback, i, j);
            Rb = (pixel >> 16) & 0xff;
            Gb = (pixel >>  8) & 0xff;
            Bb = (pixel >>  0) & 0xff;
            pixel = XGetPixel(fImage, i + x, j + y);
            if (bitmap) {
                if (pixel & 0x00FFFFFF)
                    pixel = (pixel & 0xFF000000) | fg;
                else
                    pixel = (pixel & 0xFF000000) | bg;
            }
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
    tlog("putting %ux%u+0+0 of ximage %ux%ux%u on drawable %ux%ux%u at +%d+%d\n",
          w, h, xback->width, xback->height, xback->depth, _w, _h, _d, dx-g.xorigin(), dy-g.yorigin());
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    XPutImage(xapp->display(), g.drawable(), g.handleX(), xback, 0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h);
    XDestroyImage(xback);
}

void image_init()
{
}

#endif


// vim: set sw=4 ts=4 et:
