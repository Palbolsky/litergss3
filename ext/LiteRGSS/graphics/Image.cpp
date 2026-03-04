#include <LiteCGSS2/Common/RaylibWrapper.h>
#include "LiteRGSS.h"
#include "RubyValue.h"
#include <LiteCGSS2/Common/NormalizeNumbers.h>
#include "Color.h"
#include "Image.h"

VALUE rb_cImage = Qnil;

// --- TypedData ---

static void image_free(void *ptr)
{
    auto *img = static_cast<ImageData *>(ptr);
    if (img->valid())
        raylib::UnloadImage(img->image);
    delete img;
}

static void image_mark(void *ptr) {}

static const rb_data_type_t image_type = {
    "ImageData",
    {image_mark, image_free, nullptr},
    nullptr,
    nullptr,
    RUBY_TYPED_FREE_IMMEDIATELY};

static VALUE image_alloc(VALUE klass)
{
    auto *img = new ImageData();
    return TypedData_Wrap_Struct(klass, &image_type, img);
}

ImageData *get_image(VALUE self)
{
    ImageData *img;
    TypedData_Get_Struct(self, ImageData, &image_type, img);
    return img;
}

static void check_disposed(ImageData *img)
{
    if (img->disposed)
        rb_raise(rb_eRuntimeError, "Image is disposed");
}

VALUE rb_Image_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE arg1, arg2;
    rb_scan_args(argc, argv, "11", &arg1, &arg2);
    auto *img = get_image(self);

    if (NIL_P(arg2))
    {
        rb_check_type(arg1, T_STRING);
        const char *filename = StringValueCStr(arg1);
        img->image = raylib_LoadImage(filename);
        if (!img->valid())
            rb_raise(rb_eRuntimeError, "Failed to load image from file: %s", filename);
    }
    else if (arg2 == Qtrue)
    {
        rb_check_type(arg1, T_STRING);
        unsigned char *data = (unsigned char *)RSTRING_PTR(arg1);
        int length = (int)RSTRING_LEN(arg1);
        img->image = raylib::LoadImageFromMemory(".png", data, length);
        if (!img->valid())
            rb_raise(rb_eRuntimeError, "Failed to load image from memory");
    }
    else
    {
        rb_check_type(arg1, T_FIXNUM);
        rb_check_type(arg2, T_FIXNUM);
        int w = NUM2INT(arg1);
        int h = NUM2INT(arg2);
        if (w <= 0 || h <= 0)
            rb_raise(rb_eRuntimeError, "Invalid image size (%d x %d)", w, h);
        img->image = raylib::GenImageColor(w, h, {0, 0, 0, 0});
    }

    return self;
}

VALUE rb_Image_InitializeCopy(VALUE self, VALUE other)
{
    rb_check_frozen(self);
    auto *dst = get_image(self);
    auto *src = get_image(other);
    if (src->valid())
        dst->image = raylib::ImageCopy(src->image);
    return self;
}

VALUE rb_Image_Dispose(VALUE self)
{
    auto *img = get_image(self);
    if (img->valid())
    {
        raylib::UnloadImage(img->image);
        img->image = {};
    }
    img->disposed = true;
    return self;
}

VALUE rb_Image_Disposed(VALUE self)
{
    return get_image(self)->disposed ? Qtrue : Qfalse;
}

VALUE rb_Image_Width(VALUE self)
{
    check_disposed(get_image(self));
    return INT2NUM(get_image(self)->width());
}

VALUE rb_Image_Height(VALUE self)
{
    check_disposed(get_image(self));
    return INT2NUM(get_image(self)->height());
}

VALUE rb_Image_Rect(VALUE self)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    VALUE ary = rb_ary_new_capa(4);
    rb_ary_push(ary, INT2NUM(0));
    rb_ary_push(ary, INT2NUM(0));
    rb_ary_push(ary, INT2NUM(img->width()));
    rb_ary_push(ary, INT2NUM(img->height()));
    return ary;
}

VALUE rb_Image_getPixel(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    int px = NUM2INT(x);
    int py = NUM2INT(y);
    if (px < 0 || py < 0 || px >= img->width() || py >= img->height())
        return Qnil;

    raylib::Color c = raylib::GetImageColor(img->image, px, py);
    VALUE args[4] = {INT2NUM(c.r), INT2NUM(c.g), INT2NUM(c.b), INT2NUM(c.a)};
    return rb_class_new_instance(4, args, rb_cColor);
}

VALUE rb_Image_getPixelAlpha(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    int px = NUM2INT(x);
    int py = NUM2INT(y);
    if (px < 0 || py < 0 || px >= img->width() || py >= img->height())
        return INT2NUM(0);
    return INT2NUM(raylib::GetImageColor(img->image, px, py).a);
}

VALUE rb_Image_setPixel(VALUE self, VALUE x, VALUE y, VALUE color)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    int px = NUM2INT(x);
    int py = NUM2INT(y);
    if (px < 0 || py < 0 || px >= img->width() || py >= img->height())
        return self;

    ColorData *cd = get_color_data(color);
    raylib::Color c = {cd->r, cd->g, cd->b, cd->a};
    raylib::ImageDrawPixel(&img->image, px, py, c);
    return self;
}

VALUE rb_Image_fillRect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h, VALUE color)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);

    ColorData *cd = get_color_data(color);
    raylib::Color c = {cd->r, cd->g, cd->b, cd->a};

    raylib::Rectangle rect = {
        (float)NUM2INT(x), (float)NUM2INT(y),
        (float)NUM2INT(w), (float)NUM2INT(h)};
    raylib::ImageDrawRectangleRec(&img->image, rect, c);
    return self;
}

VALUE rb_Image_clearRect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    raylib::Rectangle rect = {
        (float)NUM2INT(x), (float)NUM2INT(y),
        (float)NUM2INT(w), (float)NUM2INT(h)};
    raylib::ImageDrawRectangleRec(&img->image, rect, {0, 0, 0, 0});
    return self;
}

VALUE rb_Image_blt(VALUE self, VALUE x, VALUE y, VALUE src_image, VALUE rect)
{
    check_disposed(get_image(self));
    auto *dst = get_image(self);
    auto *src = get_image(src_image);
    if (!src->valid())
        return self;

    Check_Type(rect, T_ARRAY);
    raylib::Rectangle src_rect = {
        (float)NUM2INT(rb_ary_entry(rect, 0)),
        (float)NUM2INT(rb_ary_entry(rect, 1)),
        (float)NUM2INT(rb_ary_entry(rect, 2)),
        (float)NUM2INT(rb_ary_entry(rect, 3))};
    raylib::Rectangle dst_rect = {
        (float)NUM2INT(x), (float)NUM2INT(y),
        src_rect.width, src_rect.height};
    raylib::ImageDraw(&dst->image, src->image, src_rect, dst_rect, raylib::WHITE);
    return self;
}

VALUE rb_Image_stretchBlt(VALUE self, VALUE dst_rect, VALUE src_image, VALUE src_rect)
{
    check_disposed(get_image(self));
    auto *dst = get_image(self);
    auto *src = get_image(src_image);
    if (!src->valid())
        return self;

    Check_Type(dst_rect, T_ARRAY);
    Check_Type(src_rect, T_ARRAY);

    raylib::Rectangle srect = {
        (float)NUM2INT(rb_ary_entry(src_rect, 0)),
        (float)NUM2INT(rb_ary_entry(src_rect, 1)),
        (float)NUM2INT(rb_ary_entry(src_rect, 2)),
        (float)NUM2INT(rb_ary_entry(src_rect, 3))};
    raylib::Rectangle drect = {
        (float)NUM2INT(rb_ary_entry(dst_rect, 0)),
        (float)NUM2INT(rb_ary_entry(dst_rect, 1)),
        (float)NUM2INT(rb_ary_entry(dst_rect, 2)),
        (float)NUM2INT(rb_ary_entry(dst_rect, 3))};
    raylib::ImageDraw(&dst->image, src->image, srect, drect, raylib::WHITE);
    return self;
}

VALUE rb_Image_createMask(VALUE self, VALUE color, VALUE alpha)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    ColorData *cd = get_color_data(color);
    raylib::Color mask_color = {cd->r, cd->g, cd->b, cd->a};
    uint8_t target_alpha = (uint8_t)cgss::normalize_long(NUM2LONG(alpha), 0, 255);

    for (int py = 0; py < img->height(); py++)
    {
        for (int px = 0; px < img->width(); px++)
        {
            raylib::Color c = raylib::GetImageColor(img->image, px, py);
            if (c.r == mask_color.r && c.g == mask_color.g && c.b == mask_color.b)
            {
                c.a = target_alpha;
                raylib::ImageDrawPixel(&img->image, px, py, c);
            }
        }
    }
    return self;
}

VALUE rb_Image_toPNG(VALUE self)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    int data_size = 0;
    unsigned char *data = raylib::ExportImageToMemory(img->image, ".png", &data_size);
    if (data == nullptr)
        return Qnil;
    VALUE str = rb_str_new((const char *)data, data_size);
    raylib::MemFree(data);
    return str;
}

VALUE rb_Image_toPNGFile(VALUE self, VALUE filename)
{
    check_disposed(get_image(self));
    rb_check_type(filename, T_STRING);
    auto *img = get_image(self);
    return raylib::ExportImage(img->image, StringValueCStr(filename)) ? Qtrue : Qfalse;
}

void Init_Image()
{
    rb_cImage = rb_define_class_under(rb_mLiteRGSS, "Image", rb_cObject);
    rb_define_alloc_func(rb_cImage, image_alloc);

    rb_define_method(rb_cImage, "initialize", _rbf rb_Image_Initialize, -1);
    rb_define_method(rb_cImage, "initialize_copy", _rbf rb_Image_InitializeCopy, 1);
    rb_define_method(rb_cImage, "dispose", _rbf rb_Image_Dispose, 0);
    rb_define_method(rb_cImage, "disposed?", _rbf rb_Image_Disposed, 0);
    rb_define_method(rb_cImage, "width", _rbf rb_Image_Width, 0);
    rb_define_method(rb_cImage, "height", _rbf rb_Image_Height, 0);
    rb_define_method(rb_cImage, "rect", _rbf rb_Image_Rect, 0);
    rb_define_method(rb_cImage, "get_pixel", _rbf rb_Image_getPixel, 2);
    rb_define_method(rb_cImage, "get_pixel_alpha", _rbf rb_Image_getPixelAlpha, 2);
    rb_define_method(rb_cImage, "set_pixel", _rbf rb_Image_setPixel, 3);
    rb_define_method(rb_cImage, "fill_rect", _rbf rb_Image_fillRect, 5);
    rb_define_method(rb_cImage, "clear_rect", _rbf rb_Image_clearRect, 4);
    rb_define_method(rb_cImage, "blt", _rbf rb_Image_blt, 4);
    rb_define_method(rb_cImage, "blt!", _rbf rb_Image_blt, 4);
    rb_define_method(rb_cImage, "stretch_blt", _rbf rb_Image_stretchBlt, 3);
    rb_define_method(rb_cImage, "stretch_blt!", _rbf rb_Image_stretchBlt, 3);
    rb_define_method(rb_cImage, "create_mask", _rbf rb_Image_createMask, 2);
    rb_define_method(rb_cImage, "to_png", _rbf rb_Image_toPNG, 0);
    rb_define_method(rb_cImage, "to_png_file", _rbf rb_Image_toPNGFile, 1);
}
