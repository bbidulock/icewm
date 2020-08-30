#include "config.h"

// this has to be come before X11 headers or libpng checks will get mad WRT setjmp version
#ifdef CONFIG_LIBPNG
#include <png.h>
#endif

#if defined CONFIG_XPM && !defined(CONFIG_GDK_PIXBUF_XLIB)

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "yimage.h"
#include "yxapp.h"
#include "ypointer.h"
#include "intl.h"

#include <X11/xpm.h>

#ifdef CONFIG_LIBJPEG
#include <jpeglib.h>
#include <setjmp.h>
#endif

#define ATH 55  /* highest alpha threshold that can show anti-aliased lines */

struct Verbose {
    const bool verbose;
    Verbose() : verbose(init()) { }
    bool init() const {
#ifdef __OpenBSD__
        return false;
#else
        if (getenv("DEBUG_YXIMAGE"))
            return true;
        upath file(YApplication::getHomeDir().child(".icewm/debug_yximage"));
        return file.fileExists();
#endif
    }
    operator bool() const { return verbose; }
};

static Verbose verbose;

class YXImage: public YImage {
public:
    YXImage(XImage *ximage, bool bitmap = false) :
        YImage(ximage->width, ximage->height), fImage(ximage), fBitmap(bitmap)
    {
        // tlog("created YXImage %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
    }
    virtual ~YXImage() {
        if (fImage != 0)
            XDestroyImage(fImage);
    }
    virtual ref<YPixmap> renderToPixmap(unsigned depth, bool premult);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual bool valid() const { return fImage != 0; }
    unsigned depth() const { return fImage ? fImage->depth : 0; }
    static ref<YImage> loadxbm(upath filename);
    static ref<YImage> loadxpm(upath filename);
    static ref<YImage> loadxpm2(upath filename, int& status);
#ifdef CONFIG_LIBPNG
    static ref<YImage> loadpng(upath filename);
    static void pngload(ref<YImage>& image, FILE* f,
                        png_structp png_ptr,
                        png_infop info_ptr);
    bool savepng(upath filename, const char** error);
#endif
#ifdef CONFIG_LIBJPEG
    static ref<YImage> loadjpg(upath filename);
#endif
    static ref<YImage> combine(XImage *xdraw, XImage *xmask);
    static mstring detectImageType(upath filename);

    bool isBitmap() const { return fBitmap; }
    bool hasAlpha() const { return fImage ? fImage->depth == 32 : false; }
    ref<YImage> upscale(unsigned width, unsigned height);
    ref<YImage> downscale(unsigned width, unsigned height);
    virtual ref<YImage> subimage(int x, int y, unsigned width, unsigned height);
    virtual void save(upath filename);

    unsigned long getPixel(unsigned x, unsigned y) const {
        return XGetPixel(fImage, int(x), int(y));
    }

    static XImage* createImage(unsigned width, unsigned height, unsigned depth) {
        Visual* visual = xapp->visualForDepth(depth);
        XImage* ximage = XCreateImage(xapp->display(), visual, depth,
                                      ZPixmap, 0, NULL, width, height, 8, 0);
        if (ximage == 0)
            tlog("ERROR: could not create %ux%ux%u ximage", width, height, depth);
        else {
            ximage->data = (char *) calloc(ximage->bytes_per_line, height);
            if (ximage->data == 0) {
                tlog("ERROR: could not allocate ximage data");
                XDestroyImage(ximage);
                ximage = 0;
            }
        }
        return ximage;
    }

    static XImage* createBitmap(unsigned width, unsigned height, char* data) {
        XImage* ximage = XCreateImage(xapp->display(), xapp->visual(), 1,
                                      XYBitmap, 0, NULL, width, height, 8, 0);
        if (ximage == 0)
            tlog("ERROR: could not create %ux%u XImage mask", width, height);
        else if (data)
            ximage->data = data;
        else {
            ximage->data = (char *) calloc(ximage->bytes_per_line, height);
            if (ximage->data == 0) {
                tlog("ERROR: could not allocate %ux%u bitmap", width, height);
                XDestroyImage(ximage);
                ximage = 0;
            }
        }
        return ximage;
    }

private:
    XImage *fImage;
    bool fBitmap;
};

bool YImage::supportsDepth(unsigned depth) {
    return depth == 32 || depth == xapp->depth();
}

ref<YImage> YImage::load(upath filename)
{
    ref<YImage> image;
    mstring ext(filename.getExtension().lower());
    bool unsup = false;

    if (ext.isEmpty())
        ext = YXImage::detectImageType(filename);

    if (ext == ".xbm")
        image = YXImage::loadxbm(filename);
    else if (ext == ".xpm")
        image = YXImage::loadxpm(filename);
    else if (ext == ".png") {
#ifdef CONFIG_LIBPNG
        image = YXImage::loadpng(filename);
#else
        if (ONCE)
            warn(_("Support for PNG images was not enabled"));
#endif
    }
    else if (ext == ".jpg" || ext == ".jpeg") {
#ifdef CONFIG_LIBJPEG
        image = YXImage::loadjpg(filename);
#else
        if (ONCE)
            warn(_("Support for JPEG images was not enabled"));
#endif
    }
    else {
        unsup = true;
        if (ONCE)
            warn(_("Unsupported file format: %s"), ext.c_str());
    }

    if (image == null && !unsup)
        fail(_("Could not load image \"%s\""), filename.string());
    return image;
}

mstring YXImage::detectImageType(upath filename) {
     const int xpm = 9, png = 8, jpg = 4, len = max(xpm, png);
     char buf[len+1];
     memset(buf, 0, sizeof buf);
     if (filereader(filename.string()).read_all(buf, sizeof buf) >= len) {
         if (0 == memcmp(buf, "/* XPM */", xpm)) {
             return ".xpm";
         }
         else if (0 == memcmp(buf, "\x89PNG\x0d\x0a\x1a\x0a", png)) {
             return ".png";
         }
         else if (0 == memcmp(buf, "\377\330\377\356", jpg)) {
             return ".jpg";
         }
         else {
             msg("Unknown image type: \"%s\".", filename.string());
             return null;
         }
     }
     fail(_("Could not load image \"%s\""), filename.string());
     return null;
}

ref <YImage> YXImage::loadxbm(upath filename)
{
    ref<YImage> image;
    XImage *xdraw = 0;
    unsigned w, h;
    int x, y;
    char *data = 0;
    int status;

    status = XReadBitmapFileData(filename.string(), &w, &h, (unsigned char **) &data, &x, &y);
    if (status != BitmapSuccess) {
        tlog("ERROR: could not read pixmap file %s\n", filename.string());
        goto error;
    }
    xdraw = createBitmap(w, h, data);
    if (xdraw == 0) {
        goto error;
    }
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
    int status = XpmSuccess;
    ref<YImage> image(loadxpm2(filename, status));
    if (status != XpmFileInvalid)
        return image;

    // support themes with indirect XPM images, like OnyX:
    const int lim = 64;
    for (int k = 9; --k > 0 && inrange(int(filename.fileSize()), 5, lim); ) {
        fileptr fp(filename.fopen("r"));
        if (fp == 0)
            break;

        char buf[lim];
        if (fgets(buf, lim, fp) == 0)
            break;

        mstring match(mstring(buf).match("^[a-z][-_a-z0-9]*\\.xpm$", "i"));
        if (match == null)
            break;

        filename = filename.parent().relative(match);
        if (filename.fileSize() > lim)
            return loadxpm2(filename, status);
    }
    return image;
}

ref<YImage> YXImage::loadxpm2(upath filename, int& status)
{
    ref<YImage> image;
    XImage *xdraw = 0, *xmask = 0;
    XpmAttributes xa;

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

            ximage = createImage(w, h, 32U);
            if (ximage) {
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
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte buf[8];
    int ret;
    FILE *f;

    if (!(f = fopen(filename.string(), "rb"))) {
        tlog("could not open %s: %s\n", filename.string(), strerror(errno));
        goto nofile;
    }
    if ((ret = fread(buf, 1, 8, f)) != 8) {
        tlog("could not read PNG signature %s: %s\n", filename.string(), strerror(errno));
        goto noread;
    }
    if (png_sig_cmp(buf, 0, 8)) {
        tlog("invalid PNG signature in %s\n", filename.string());
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

    pngload(image, f, png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    goto noread;
  noinfo:
    png_destroy_read_struct(&png_ptr, NULL, NULL);
  noread:
    fclose(f);
  nofile:
    return image;
}

void YXImage::pngload(ref<YImage>& image, FILE* f,
                      png_structp png_ptr,
                      png_infop info_ptr)
{
    if (setjmp(png_jmpbuf(png_ptr))) {
        tlog("ERROR: longjump from setjump\n");
    } else {
        png_uint_32 width, height, row_bytes, i, j;
        int bit_depth, color_type, channels;

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
        png_byte *png_pixels = static_cast<png_byte *>(
                  calloc(row_bytes * height, sizeof(*png_pixels)));
        png_byte **row_pointers = static_cast<png_byte **>(
                   calloc(height, sizeof(*row_pointers)));
        for (i = 0; i < height; i++)
            row_pointers[i] = png_pixels + i * row_bytes;
        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, info_ptr);
        XImage *ximage = createImage(width, height, 32U);
        if (ximage) {
            unsigned long pixel, A = 0, R = 0, G = 0, B = 0;
            png_byte *p = png_pixels;
            for (j = 0; j < height; j++) {
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

            image.init(new YXImage(ximage));
        }
        if (png_pixels)
            free(png_pixels);
        if (row_pointers)
            free(row_pointers);
    }
}
#endif

void YXImage::save(upath filename) {
#ifdef CONFIG_LIBPNG
    filename = filename.replaceExtension(".png");
    const char* error = "";
    if (savepng(filename, &error) == false) {
        fail("Cannot write YXImage %s: %s",
            filename.string(), error);
    }
#endif
}

#ifdef CONFIG_LIBPNG
bool YXImage::savepng(upath filename, const char** error) {
    const unsigned width(this->width());
    const unsigned height(this->height());
    bool saved = false;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    asmart<png_byte> row(new png_byte[4 * width * sizeof(png_byte)]);

    fileptr fp(filename.fopen("w"));
    if (fp == NULL) {
        *error = strerror(errno);
        goto end;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        *error = "Cannot create PNG write struct";
        goto end;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        *error = "Cannot create PNG info struct";
        goto end;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        *error = "Error during PNG file output";
        saved = false;
        goto end;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr,
                 width, height, 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned long pixel = this->getPixel(x, y);
            row[x * 4 + 0] = pixel >> 16 & 0xFF;
            row[x * 4 + 1] = pixel >>  8 & 0xFF;
            row[x * 4 + 2] = pixel >>  0 & 0xFF;
            row[x * 4 + 3] = max(pixel >> 24 & 0xFF, 0x80UL);
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    saved = true;

end:
    if (info_ptr)
        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr)
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

    return saved;
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
        fail("could not open %s", filename.string());
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
    switch (cinfo.out_color_space) {
        case JCS_RGB:
#ifdef JCS_EXTENSIONS
        case JCS_EXT_RGBA:
#endif
        case JCS_GRAYSCALE:
            break;
        default:
            warn("JPEG color space %d is not yet supported; please report this",
                    cinfo.out_color_space);
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return null;
    }
    (void) jpeg_start_decompress(&cinfo);
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    const int width = int(cinfo.output_width);
    const int height = int(cinfo.output_height);
    XImage* ximage = createImage(width, height, 32U);
    if (ximage) {
        const bool bigendian = ximage->byte_order == MSBFirst;
        const int colorspace = cinfo.out_color_space;
        const int bpp = cinfo.num_components;

        for (int line; (line = int(cinfo.output_scanline)) < height; ) {
            (void) jpeg_read_scanlines(&cinfo, buffer, 1);
            unsigned char* buf = (unsigned char *) buffer[0];
            unsigned char* dst = (unsigned char *) ximage->data
                               + line * ximage->bytes_per_line;
            if (bigendian) {
                if (colorspace == JCS_RGB) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[0] = 0xFF;
                        dst[1] = buf[0];
                        dst[2] = buf[1];
                        dst[3] = buf[2];
                    }
                }
#ifdef JCS_EXTENSIONS
                else if (colorspace == JCS_EXT_RGBA) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[0] = buf[3];
                        dst[1] = buf[0];
                        dst[2] = buf[1];
                        dst[3] = buf[2];
                    }
                }
#endif
                else if (colorspace == JCS_GRAYSCALE) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[0] = 0xFF;
                        dst[1] = buf[0];
                        dst[2] = buf[0];
                        dst[3] = buf[0];
                    }
                }
            } else {
                if (colorspace == JCS_RGB) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[3] = 0xFF;
                        dst[2] = buf[0];
                        dst[1] = buf[1];
                        dst[0] = buf[2];
                    }
                }
#ifdef JCS_EXTENSIONS
                else if (colorspace == JCS_EXT_RGBA) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[3] = buf[3];
                        dst[2] = buf[0];
                        dst[1] = buf[1];
                        dst[0] = buf[2];
                    }
                }
#endif
                else if (colorspace == JCS_GRAYSCALE) {
                    for (int i = 0; i < width; ++i, buf += bpp, dst += 4) {
                        dst[3] = 0xFF;
                        dst[2] = buf[0];
                        dst[1] = buf[0];
                        dst[0] = buf[0];
                    }
                }
            }
        }
        yximage.init(new YXImage(ximage));
    }

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
        bool has_alpha = hasAlpha();

        // tlog("upscale from %ux%ux%u to %ux%u\n", w, h, d, nw, nh);
        ximage = createImage(nw, nh, fImage->depth);
        if (ximage == 0) {
            goto error;
        }
        // tlog("created upscale ximage at %ux%ux%u\n", ximage->width, ximage->height, ximage->depth);
        if (!(chanls = (double *)calloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(counts = (double *)calloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
            tlog("ERROR: could not allocate working arrays\n");
            goto error;
        }
        if (!(colors = (double *)calloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
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
                            double xf = 1.0;
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

// image downscaling using a fast bilinear interpolation algorithm
ref<YImage> YXImage::downscale(unsigned newWidth, unsigned newHeight)
{
    const unsigned oldWidth = this->width();
    const unsigned oldHeight = this->height();
    const unsigned shift = 10;

    PRECONDITION(inrange(newWidth, 1U, oldWidth));
    PRECONDITION(inrange(newHeight, 1U, oldHeight));

    XImage* ximage = createImage(newWidth, newHeight, 32U);
    if (ximage == 0)
        return null;

    unsigned long *red = new unsigned long[newWidth];
    unsigned long *grn = new unsigned long[newWidth];
    unsigned long *blu = new unsigned long[newWidth];
    unsigned long *alp = new unsigned long[newWidth];
    unsigned long *div = new unsigned long[newWidth];

    unsigned hacc = 0;
    unsigned h = 0;
    unsigned mult = 0;
    bool repeat = false;

    for (unsigned y = 0; y < oldHeight; y = repeat ? y : 1 + y) {
        if (hacc < newHeight) {
            memset(red, 0, sizeof(*red) * newWidth);
            memset(grn, 0, sizeof(*grn) * newWidth);
            memset(blu, 0, sizeof(*blu) * newWidth);
            memset(alp, 0, sizeof(*alp) * newWidth);
            memset(div, 0, sizeof(*div) * newWidth);
        }

        if (repeat) {
            repeat = false;
            mult = (1 << shift) - mult;
        }
        else {
            hacc += newHeight;
            if (hacc <= oldHeight) {
                mult = 1 << shift;
            }
            else {
                mult = ((newHeight / 2) + (1 << shift)
                     * (newHeight - (hacc - oldHeight)))
                     / newHeight;
            }
        }

        unsigned wacc = 0;
        unsigned w = 0;
        unsigned char* idata = (unsigned char *) fImage->data
                               + y * fImage->bytes_per_line;
        const bool has_alpha = hasAlpha();
        for (unsigned x = 0; x < oldWidth; ++x) {
            unsigned char r, g, b, a;
            if (fImage->byte_order == MSBFirst) {
                a = has_alpha ? *idata++ : 0xFF;
                r = *idata++;
                g = *idata++;
                b = *idata++;
            } else {
                b = *idata++;
                g = *idata++;
                r = *idata++;
                a = has_alpha ? *idata++ : 0xFF;
            }

            wacc += newWidth;
            if (wacc < oldWidth) {
                red[w] += (mult << shift) * r;
                grn[w] += (mult << shift) * g;
                blu[w] += (mult << shift) * b;
                alp[w] += (mult << shift) * a;
                div[w] += mult << shift;
            }
            else {
                unsigned m = (newWidth / 2 + (1 << shift)
                           * (newWidth - (wacc - oldWidth)))
                           / newWidth;
                red[w] += m * mult * r;
                grn[w] += m * mult * g;
                blu[w] += m * mult * b;
                alp[w] += m * mult * a;
                div[w] += m * mult;
                ++w;
                wacc -= oldWidth;
                if (wacc > 0) {
                    m = (1 << shift) - m;
                    red[w] += m * mult * r;
                    grn[w] += m * mult * g;
                    blu[w] += m * mult * b;
                    alp[w] += m * mult * a;
                    div[w] += m * mult;
                }
            }
        }

        if (hacc >= oldHeight) {
            hacc -= oldHeight;
            if (hacc > 0) {
                repeat = true;
            }
            unsigned char* odata = (unsigned char *) ximage->data
                                   + h * ximage->bytes_per_line;
            for (unsigned k = 0; k < newWidth; ++k) {
                unsigned long d = non_zero(div[k]);
                if (ximage->byte_order == MSBFirst) {
                    *odata++ = (unsigned char) (alp[k] / d);
                    *odata++ = (unsigned char) (red[k] / d);
                    *odata++ = (unsigned char) (grn[k] / d);
                    *odata++ = (unsigned char) (blu[k] / d);
                }
                else {
                    *odata++ = (unsigned char) (blu[k] / d);
                    *odata++ = (unsigned char) (grn[k] / d);
                    *odata++ = (unsigned char) (red[k] / d);
                    *odata++ = (unsigned char) (alp[k] / d);
                }
            }
            ++h;
        }
    }

    delete[] red;
    delete[] grn;
    delete[] blu;
    delete[] alp;
    delete[] div;

    return ref<YImage>(new YXImage(ximage));
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
    if (!valid()) {
        tlog("invalid YXImage\n");
        return null;
    }

    unsigned w = fImage->width;
    unsigned h = fImage->height;
    if (nw == w && nh == h)
        return ref<YImage>(this);
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
        tlog("could not get geometry of pixmap 0x%lx\n", pixmap);
        return image;
    }

    if (verbose)
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
    ximage = createImage(w, h, 32U);
    if (ximage == 0) {
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
    ximage = YXImage::createImage(w, h, 32U);
    if (ximage == 0) {
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

ref <YPixmap> YXImage::renderToPixmap(unsigned depth, bool premult)
{
    ref <YPixmap> pixmap;
    bool has_mask = false;
    XImage *xdraw = 0, *xmask = 0;
    Pixmap draw = None, mask = None;
    XGCValues xg;
    GC gcd = None, gcm = None;

    if (depth == 0) {
        depth = xapp->depth();
    }
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
        if (hasAlpha() || depth != this->depth()) {
            xdraw = createImage(w, h, depth);
            if (xdraw == 0) {
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

        xmask = createBitmap(w, h, NULL);
        if (xmask == 0) {
            goto error;
        }
        for (unsigned j = 0; j < h; j++)
            for (unsigned i = 0; i < w; i++)
                XPutPixel(xmask, i, j,
                          !has_mask ||
                          ((XGetPixel(fImage, i, j) >> 24) & 0xff) >= ATH);
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
    unsigned fg = g.color() ? g.color().pixel() & 0x00FFFFFF : 0;
    unsigned bg = 0x00000000; /* for now */
    unsigned backAlpha = (g.rdepth() == 32) ? 0 : 0xFF;
    unsigned backAmask = (g.rdepth() == 32) ? 0xFF : 0;
    unsigned foreAlpha = (depth() == 32) ? 0 : 0xFF;
    unsigned foreAmask = (depth() == 32) ? 0xFF : 0;

    if (verbose)
    tlog("compositing %ux%u+%d+%d of %ux%ux%u onto drawable 0x%lx at +%d+%d\n", w, h, x, y, wi, hi, di, g.drawable(), dx, dy);
    Window root;
    int _x, _y;
    unsigned _w, _h, _b, _d;

    if (XGetGeometry(xapp->display(), g.drawable(), &root, &_x, &_y, &_w, &_h, &_b, &_d))
        if (verbose)
        tlog("drawable 0x%lx has geometry %ux%ux%u+%d+%d\n", g.drawable(), _w, _h, _d, _x, _y);
    if (g.xorigin() > dx) {
        if ((int) w <= g.xorigin() - dx) {
            if (verbose)
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if ((int) h <= g.xorigin() - dx) {
            if (verbose)
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if ((int) (dx + w) > (int) (g.xorigin() + g.rwidth())) {
        if ((int) (g.xorigin() + g.rwidth()) <= dx) {
            if (verbose)
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        w = g.xorigin() + g.rwidth() - dx;
    }
    if ((int) (dy + h) > (int) (g.yorigin() + g.rheight())) {
        if ((int) (g.yorigin() + g.rheight()) <= dy) {
            if (verbose)
            tlog("ERROR: coordinates out of bounds\n");
            return;
        }
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0) {
        if (verbose)
        tlog("ERROR: coordinates out of bounds\n");
        return;
    }
    if (!hasAlpha()) {
        if (verbose)
        tlog("simply putting %ux%u+0+0 of ximage %ux%ux%u onto drawable %ux%ux%u at +%d+%d\n",
              w, h, wi, hi, di, _w, _h, _d, dx-g.xorigin(), dy-g.yorigin());
        // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
        XPutImage(xapp->display(), g.drawable(), g.handleX(), fImage,  0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h);
        return;
    }

    if (verbose)
    tlog("getting image %ux%u+%d+%d from drawable %ux%ux%u\n", w, h, dx-g.xorigin(), dy-g.yorigin(), _w, _h, _d);
    // tlog("next request %lu at %s: +%d : %s()\n", NextRequest(xapp->display()), __FILE__, __LINE__, __func__);
    xback = XGetImage(xapp->display(), g.drawable(), dx - g.xorigin(), dy - g.yorigin(), w, h, AllPlanes, ZPixmap);
    if (!xback) {
        tlog("ERROR: could not get backing image\n");
        xback = createImage(w, h, g.rdepth());
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
            unsigned Rb, Gb, Bb, Ab, A, R, G, B;
            unsigned pixel;

            pixel = XGetPixel(xback, i, j);
            Rb = (pixel >> 16) & 0xff;
            Gb = (pixel >>  8) & 0xff;
            Bb = (pixel >>  0) & 0xff;
            Ab = ((pixel >> 24) & backAmask) | backAlpha;
            pixel = XGetPixel(fImage, i + x, j + y);
            if (bitmap) {
                if (pixel & 0x00FFFFFF)
                    pixel = (pixel & 0xFF000000) | fg;
                else
                    pixel = (pixel & 0xFF000000) | bg;
            }
            A = ((pixel >> 24) & foreAmask) | foreAlpha;
            R = (pixel >> 16) & 0xff;
            G = (pixel >>  8) & 0xff;
            B = (pixel >>  0) & 0xff;

            Rb = ((A * R) + ((255 - A) * Rb)) / 255;
            Gb = ((A * G) + ((255 - A) * Gb)) / 255;
            Bb = ((A * B) + ((255 - A) * Bb)) / 255;
            Ab = (A + Ab + 1) / 2;
            pixel = (Rb << 16)|(Gb << 8)|(Bb << 0)|(Ab << 24);
            XPutPixel(xback, i, j, pixel);
        }
    }

    if (verbose)
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
