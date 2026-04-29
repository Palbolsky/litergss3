// Yuki::GifReader — animated GIF decoder, wraps cgss::YukiGif. Lifecycle
// methods (initialize/width/height/frame/frame_count) are wired. The
// `update` and `draw` methods take a Bitmap (Image alias) target, but
// cgss::YukiGif::drawOn/update expect a cgss::Texture — litergss3's Image
// is cgss::Image (CPU pixels) without a Texture bridge in this pass. Both
// methods raise NotImplementedError until that bridge lands. PSDK code
// using GifReader for animation playback won't render until then.

#include "Yuki.h"
#include "LiteRGSS.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Image/YukiGif.h>

VALUE rb_cYukiGifReader = Qnil;
VALUE rb_cYukiGifError = Qnil;

struct YukiGifData
{
    cgss::YukiGif gif;
    bool loaded = false;
};

static YukiGifData *get_gif(VALUE self) { return rb::GetPtr<YukiGifData>(self); }

VALUE rb_Yuki_GifReader_Initialize(int argc, VALUE *argv, VALUE self)
{
    auto *g = get_gif(self);
    VALUE str, from_memory;
    rb_scan_args(argc, argv, "11", &str, &from_memory);
    rb_check_type(str, T_STRING);

    if (RTEST(from_memory)) {
        rb_str_freeze(str);
        rb_iv_set(self, "@__gif_data", str);
        if (!g->gif.load(RSTRING_PTR(str), static_cast<std::size_t>(RSTRING_LEN(str)))) {
            rb_raise(rb_cYukiGifError, "Failed to load GIF from Memory");
        }
    } else {
        VALUE file = rb_file_open(RSTRING_PTR(str), "rb");
        VALUE size = rb_funcall(file, rb_intern("size"), 0);
        VALUE data = rb_funcall(file, rb_intern("read"), 1, size);
        rb_io_close(file);
        rb_str_freeze(data);
        rb_iv_set(self, "@__gif_data", data);
        if (!g->gif.load(RSTRING_PTR(data), static_cast<std::size_t>(RSTRING_LEN(data)))) {
            rb_raise(rb_cYukiGifError, "Failed to load GIF from File (%s)", RSTRING_PTR(str));
        }
    }
    g->loaded = true;
    return self;
}

VALUE rb_Yuki_GifReader_Draw(VALUE self, VALUE texture)
{
    (void)self; (void)texture;
    rb_raise(rb_eNotImpError,
             "Yuki::GifReader#draw requires a cgss::Texture target — "
             "litergss3's Bitmap (Image) does not yet expose a GPU texture handle.");
    return self;
}

VALUE rb_Yuki_GifReader_Update(VALUE self, VALUE texture)
{
    (void)self; (void)texture;
    rb_raise(rb_eNotImpError,
             "Yuki::GifReader#update requires a cgss::Texture target — see draw.");
    return self;
}

VALUE rb_Yuki_GifReader_Width(VALUE self) { return UINT2NUM(get_gif(self)->gif.width()); }
VALUE rb_Yuki_GifReader_Height(VALUE self) { return UINT2NUM(get_gif(self)->gif.height()); }
VALUE rb_Yuki_GifReader_Frame(VALUE self) { return ULONG2NUM(get_gif(self)->gif.frame()); }
VALUE rb_Yuki_GifReader_FrameSet(VALUE self, VALUE frame) { get_gif(self)->gif.setFrame(NUM2ULONG(frame)); return frame; }
VALUE rb_Yuki_GifReader_FrameCount(VALUE self) { return ULONG2NUM(get_gif(self)->gif.frameCount()); }
VALUE rb_Yuki_GifReader_SetDeltaCounter(VALUE self, VALUE delta) { (void)self; cgss::YukiGif::FrameDelta() = NUM2DBL(delta); return delta; }
VALUE rb_Yuki_GifReader_Copy(VALUE self) { (void)self; rb_raise(rb_eRGSSError, "Gif cannot be cloned or duplicated."); return self; }

void Init_YukiGifReader()
{
    rb_cYukiGifReader = rb_define_class_under(rb_mYuki, "GifReader", rb_cObject);
    rb_define_alloc_func(rb_cYukiGifReader, rb::Alloc<YukiGifData>);
    rb_cYukiGifError = rb_define_class_under(rb_cYukiGifReader, "Error", rb_eStandardError);

    rb_define_method(rb_cYukiGifReader, "initialize", _rbf rb_Yuki_GifReader_Initialize, -1);
    rb_define_method(rb_cYukiGifReader, "update", _rbf rb_Yuki_GifReader_Update, 1);
    rb_define_method(rb_cYukiGifReader, "draw", _rbf rb_Yuki_GifReader_Draw, 1);
    rb_define_method(rb_cYukiGifReader, "width", _rbf rb_Yuki_GifReader_Width, 0);
    rb_define_method(rb_cYukiGifReader, "height", _rbf rb_Yuki_GifReader_Height, 0);
    rb_define_method(rb_cYukiGifReader, "frame", _rbf rb_Yuki_GifReader_Frame, 0);
    rb_define_method(rb_cYukiGifReader, "frame=", _rbf rb_Yuki_GifReader_FrameSet, 1);
    rb_define_method(rb_cYukiGifReader, "frame_count", _rbf rb_Yuki_GifReader_FrameCount, 0);
    rb_define_method(rb_cYukiGifReader, "clone", _rbf rb_Yuki_GifReader_Copy, 0);
    rb_define_method(rb_cYukiGifReader, "dup", _rbf rb_Yuki_GifReader_Copy, 0);
    rb_define_singleton_method(rb_cYukiGifReader, "delta_counter=", _rbf rb_Yuki_GifReader_SetDeltaCounter, 1);
}
