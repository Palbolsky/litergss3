// Backend-agnostic Ruby Image binding. Routes all operations through
// cgss::Image + backend Ops — no raylib:: or sf:: symbols reach this TU.
// File I/O uses the ImageFileSerializer family; PNG-bytes export uses
// ImageMemorySerializer.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Image/Serializers/ImageSerializer.h>
#include "Color.h"
#include "Image.h"
#include "Rect.h"
#include "DrawableDisposable.h"

VALUE rb_cImage = Qnil;

ImageData *get_image(VALUE self) { return rb::GetPtr<ImageData>(self); }

static void check_disposed(ImageData *img)
{
    if (img->disposed)
        rb_raise(rb_eRuntimeError, "Image is disposed");
}

// Internal: any operation that mutates `image` calls this so the next
// `image_acquire_texture` re-syncs the GPU side. Cheap to call
// repeatedly — actual work is deferred until a drawable asks for the
// texture.
static void mark_image_dirty(ImageData *img)
{
    img->textureDirty = true;
}

cgss::Texture& image_acquire_texture(ImageData *img)
{
    if (!img->textureValid) {
        img->cachedTexture = cgss::Texture::create(img->image);
        img->textureValid = true;
        img->textureDirty = false;
    } else if (img->textureDirty) {
        // Re-upload pixels into the existing GPU texture rather than
        // reallocating — preserves the shared_ptr<native_texture> identity
        // so any Sprite/SpriteMap that already cached a pointer to the
        // native handle keeps working. cgss::Texture::update(image) is the
        // dedicated re-upload path.
        img->cachedTexture.update(img->image);
        img->textureDirty = false;
    }
    return img->cachedTexture;
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
        img->image = cgss::Image::create(std::string{filename});
        if (!img->valid())
            rb_raise(rb_eRuntimeError, "Failed to load image from file: %s", filename);
    }
    else if (arg2 == Qtrue)
    {
        rb_check_type(arg1, T_STRING);
        const char *data = RSTRING_PTR(arg1);
        const std::size_t length = static_cast<std::size_t>(RSTRING_LEN(arg1));
        img->image = cgss::Image::create(data, length);
        if (!img->valid())
            rb_raise(rb_eRuntimeError, "Failed to load image from memory");
    }
    else
    {
        rb_check_type(arg1, T_FIXNUM);
        rb_check_type(arg2, T_FIXNUM);
        const int w = NUM2INT(arg1);
        const int h = NUM2INT(arg2);
        if (w <= 0 || h <= 0)
            rb_raise(rb_eRuntimeError, "Invalid image size (%d x %d)", w, h);
        img->image = cgss::Image::create(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    }

    return self;
}

VALUE rb_Image_InitializeCopy(VALUE self, VALUE other)
{
    rb_check_frozen(self);
    auto *dst = get_image(self);
    auto *src = get_image(other);
    if (src->valid()) {
        dst->image = src->image;  // cgss::Image copy-assign = deep copy of pixels
        // The GPU texture wasn't copied — invalidate so the next acquire
        // uploads from the freshly-copied CPU pixels. (Don't share the
        // src's cached texture: the dst is logically independent and the
        // user may mutate it without expecting src to follow.)
        dst->textureValid = false;
        dst->textureDirty = false;
    }
    return self;
}

VALUE rb_Image_Dispose(VALUE self)
{
    auto *img = get_image(self);
    img->image = cgss::Image{};   // replacing triggers old image's destructor
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
    return UINT2NUM(get_image(self)->width());
}

VALUE rb_Image_Height(VALUE self)
{
    check_disposed(get_image(self));
    return UINT2NUM(get_image(self)->height());
}

VALUE rb_Image_Rect(VALUE self)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    VALUE ary = rb_ary_new_capa(4);
    rb_ary_push(ary, INT2NUM(0));
    rb_ary_push(ary, INT2NUM(0));
    rb_ary_push(ary, UINT2NUM(img->width()));
    rb_ary_push(ary, UINT2NUM(img->height()));
    return ary;
}

VALUE rb_Image_getPixel(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    const int px = NUM2INT(x);
    const int py = NUM2INT(y);
    if (px < 0 || py < 0) return Qnil;
    const auto pixel = img->image.getPixel(static_cast<unsigned int>(px), static_cast<unsigned int>(py));
    if (!pixel) return Qnil;
    VALUE args[4] = { INT2NUM(pixel->r), INT2NUM(pixel->g), INT2NUM(pixel->b), INT2NUM(pixel->a) };
    return rb_class_new_instance(4, args, rb_cColor);
}

VALUE rb_Image_getPixelAlpha(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    const int px = NUM2INT(x);
    const int py = NUM2INT(y);
    if (px < 0 || py < 0) return INT2NUM(0);
    const auto pixel = img->image.getPixel(static_cast<unsigned int>(px), static_cast<unsigned int>(py));
    return INT2NUM(pixel ? pixel->a : 0);
}

VALUE rb_Image_setPixel(VALUE self, VALUE x, VALUE y, VALUE color)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    const int px = NUM2INT(x);
    const int py = NUM2INT(y);
    if (px < 0 || py < 0) return self;
    ColorData *cd = get_color_data(color);
    img->image.setPixel(static_cast<unsigned int>(px), static_cast<unsigned int>(py),
                        cgss::Color{cd->r, cd->g, cd->b, cd->a});
    mark_image_dirty(img);
    return self;
}

VALUE rb_Image_fillRect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h, VALUE color)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    ColorData *cd = get_color_data(color);
    img->image.fillRect(cgss::Color{cd->r, cd->g, cd->b, cd->a},
                        static_cast<unsigned int>(NUM2INT(x)),
                        static_cast<unsigned int>(NUM2INT(y)),
                        static_cast<unsigned int>(NUM2INT(w)),
                        static_cast<unsigned int>(NUM2INT(h)));
    mark_image_dirty(img);
    return self;
}

VALUE rb_Image_clearRect(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    img->image.fillRect(cgss::Colors::Transparent,
                        static_cast<unsigned int>(NUM2INT(x)),
                        static_cast<unsigned int>(NUM2INT(y)),
                        static_cast<unsigned int>(NUM2INT(w)),
                        static_cast<unsigned int>(NUM2INT(h)));
    mark_image_dirty(img);
    return self;
}

// Helper: extract (x, y, w, h) from either an Array or a LiteRGSS::Rect.
// PSDK code passes both forms (LiteRGSS2's `Rect.new(...)` produced a Rect
// object, but several call sites pass `[x, y, w, h]` directly).
static cgss::IntRect rect_arg_to_intrect(VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *r = get_rect_data(val);
        return cgss::IntRect{ r->x, r->y, r->width, r->height };
    }
    Check_Type(val, T_ARRAY);
    return cgss::IntRect{
        NUM2INT(rb_ary_entry(val, 0)),
        NUM2INT(rb_ary_entry(val, 1)),
        NUM2INT(rb_ary_entry(val, 2)),
        NUM2INT(rb_ary_entry(val, 3))};
}

VALUE rb_Image_blt(VALUE self, VALUE x, VALUE y, VALUE src_image, VALUE rect)
{
    check_disposed(get_image(self));
    auto *dst = get_image(self);
    auto *src = get_image(src_image);
    if (!src->valid()) return self;

    cgss::IntRect src_rect = rect_arg_to_intrect(rect);
    dst->image.blit(src->image,
                    static_cast<unsigned int>(NUM2INT(x)),
                    static_cast<unsigned int>(NUM2INT(y)),
                    src_rect);
    mark_image_dirty(dst);
    return self;
}

VALUE rb_Image_stretchBlt(VALUE self, VALUE dst_rect, VALUE src_image, VALUE src_rect)
{
    check_disposed(get_image(self));
    auto *dst = get_image(self);
    auto *src = get_image(src_image);
    if (!src->valid()) return self;

    cgss::IntRect drect = rect_arg_to_intrect(dst_rect);
    cgss::IntRect srect = rect_arg_to_intrect(src_rect);
    dst->image.stretchBlit(src->image, drect, srect);
    mark_image_dirty(dst);
    return self;
}

// LiteRGSS2's `image.copy_to_bitmap(dst)` blitted the whole image into
// `dst` at (0, 0). PSDK uses this to materialise an Image into a Bitmap
// (the GPU-resident texture) — since LiteRGSS3 unifies Bitmap with Image,
// it's just `dst.blt!(0, 0, self, self.rect)`.
VALUE rb_Image_copyToBitmap(VALUE self, VALUE dst)
{
    check_disposed(get_image(self));
    auto *src = get_image(self);
    if (!src->valid()) return self;
    auto *d = get_image(dst);
    cgss::IntRect r{ 0, 0, static_cast<int>(src->width()), static_cast<int>(src->height()) };
    d->image.blit(src->image, 0u, 0u, r);
    mark_image_dirty(d);
    return self;
}

VALUE rb_Image_createMask(VALUE self, VALUE color, VALUE alpha)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);
    ColorData *cd = get_color_data(color);
    const auto target_alpha = static_cast<unsigned char>(
        cgss::normalize_long(NUM2LONG(alpha), 0, 255));
    img->image.createMaskFromColor(cgss::Color{cd->r, cd->g, cd->b, cd->a}, target_alpha);
    mark_image_dirty(img);
    return self;
}

VALUE rb_Image_toPNG(VALUE self)
{
    check_disposed(get_image(self));
    auto *img = get_image(self);

    // shouldFree=true → the serializer's destructor releases the buffer
    // allocated by basic_ImageMemorySerializeMethod::save.
    cgss::ImageMemorySerializer saver{ { nullptr, 0u }, true };
    // write() delegates to the saver's Saver<>::save — which stores the
    // PNG-encoded bytes into the serializer's internal buffer.
    if (img->image.write(saver) != 0u) return Qnil;

    VALUE result = Qnil;
    saver.finalizeMemory([&result](const cgss::MemorySerializerData& mem) {
        if (mem.first != nullptr && mem.second > 0) {
            result = rb_str_new(reinterpret_cast<const char*>(mem.first),
                                static_cast<long>(mem.second));
        }
    });
    return result;
}

// LiteRGSS2's Bitmap#update committed CPU-side edits to the GPU texture.
// Same contract here: flag the cached texture dirty so the next drawable
// that acquires it re-uploads. Cheap; the actual upload happens lazily.
VALUE rb_Image_update(VALUE self)
{
    auto *img = get_image(self);
    if (img == nullptr || img->disposed) return self;
    mark_image_dirty(img);
    return self;
}

VALUE rb_Image_toPNGFile(VALUE self, VALUE filename)
{
    check_disposed(get_image(self));
    rb_check_type(filename, T_STRING);
    auto *img = get_image(self);
    cgss::ImageFileSerializer saver{ std::string{StringValueCStr(filename)} };
    return img->image.write(saver) == 0u ? Qtrue : Qfalse;
}

void Init_Image()
{
    rb_cImage = rb_define_class_under(rb_mLiteRGSS, "Image", rb_cDisposable);
    rb_define_alloc_func(rb_cImage, rb::Alloc<ImageData>);

    rb_define_method(rb_cImage, "initialize", _rbf rb_Image_Initialize, -1);
    rb_define_method(rb_cImage, "initialize_copy", _rbf rb_Image_InitializeCopy, 1);
    rb_define_method(rb_cImage, "dispose", _rbf rb_Image_Dispose, 0);
    rb_define_method(rb_cImage, "disposed?", _rbf rb_Image_Disposed, 0);
    rb_define_method(rb_cImage, "update", _rbf rb_Image_update, 0);
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
    rb_define_method(rb_cImage, "copy_to_bitmap", _rbf rb_Image_copyToBitmap, 1);
    rb_define_method(rb_cImage, "create_mask", _rbf rb_Image_createMask, 2);
    rb_define_method(rb_cImage, "to_png", _rbf rb_Image_toPNG, 0);
    rb_define_method(rb_cImage, "to_png_file", _rbf rb_Image_toPNGFile, 1);
}
