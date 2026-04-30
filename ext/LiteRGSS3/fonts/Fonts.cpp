// Backend-agnostic Ruby Fonts binding. Font storage uses cgss::Font; all
// load/destroy routes through backend Ops. The per-font atlas release is
// handled by cgss::Font's destructor (which calls Ops::font_destroy on
// the last-held instance).

#include <iostream>
#include <string>
#include <vector>

#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Graphics/Font.h>

#include "LiteRGSS.h"
#include "Color.h"
#include "Fonts.h"

VALUE rb_mFonts = Qnil;

ID rb_Fonts_ivSize = Qnil;
ID rb_Fonts_ivFColor = Qnil;
ID rb_Fonts_ivOColor = Qnil;
ID rb_Fonts_ivSColor = Qnil;

std::vector<unsigned int> rb_Fonts_Size_Tbl;
// cgss::Font is non-copy-assignable in practice (the copy-ctor does a
// deep copy of the native font, which is expensive for SFML's font-face
// data and non-trivial for raylib's GPU atlas). Store as unique_ptr so
// vector re-sizing moves pointers rather than copying Fonts.
static std::vector<std::unique_ptr<cgss::Font>> g_fontTable;
// Original .ttf path per font_id. Retained so set_default_size can rebuild
// the raylib GPU atlas at the exact requested pixel size — pixel fonts
// (PSDK's PowerGreenSmall etc.) need this to render 1:1 instead of being
// downscaled from raylib's default LoadFont base size of 32.
static std::vector<std::string> g_fontPaths;

VALUE rb_Fonts_clear_all(VALUE self)
{
    rb_Fonts_Size_Tbl.clear();
    rb_Fonts_Size_Tbl.shrink_to_fit();
    // Dropping the unique_ptrs triggers each cgss::Font's dtor, which
    // routes through Ops::font_destroy to release the GPU atlas / SFML
    // font-face backing.
    g_fontTable.clear();
    g_fontTable.shrink_to_fit();
    g_fontPaths.clear();
    g_fontPaths.shrink_to_fit();
    return self;
}

// Internal helper: (re)build the GPU atlas for a font slot. If a default
// size is registered for the slot, build at that pixel size — otherwise
// fall back to the backend default (raylib LoadFont @ 32, SFML on-demand).
static void rb_Fonts_rebuild_atlas(unsigned long position)
{
    if (position >= g_fontTable.size() || !g_fontTable[position]) return;
    if (position >= g_fontPaths.size() || g_fontPaths[position].empty()) return;
    const auto& path = g_fontPaths[position];
    const bool has_size = position < rb_Fonts_Size_Tbl.size() && rb_Fonts_Size_Tbl[position] > 0;
    if (has_size) {
        g_fontTable[position]->load(path, rb_Fonts_Size_Tbl[position]);
    } else {
        g_fontTable[position]->load(path);
    }
}

VALUE rb_Fonts_load_font(VALUE self, VALUE id, VALUE str)
{
    const unsigned long position = cgss::normalize_long(rb_num2long(id), 0, 255);
    rb_check_type(str, T_STRING);
    while (g_fontTable.size() <= position) {
        g_fontTable.push_back(std::make_unique<cgss::Font>());
    }
    while (g_fontPaths.size() <= position) {
        g_fontPaths.emplace_back();
    }
    g_fontPaths[position] = std::string{RSTRING_PTR(str)};
    // Rebuild (uses set_default_size's value when set_default_size was
    // already called for this slot — PSDK calls them back-to-back per
    // font, in either order).
    rb_Fonts_rebuild_atlas(position);
    return self;
}

cgss::Font& rb_Fonts_get_font(unsigned long id)
{
    if (g_fontTable.size() <= id || !g_fontTable[id]) {
        rb_raise(rb_eRGSSError, "Unable to fetch font n°%d.", static_cast<int>(id));
    }
    return *g_fontTable[id];
}

VALUE rb_Fonts_set_default_size(VALUE self, VALUE id, VALUE size)
{
    const unsigned long position = rb_num2long(id);
    while (rb_Fonts_Size_Tbl.size() <= position) {
        rb_Fonts_Size_Tbl.push_back(16);
    }
    rb_Fonts_Size_Tbl[position] = static_cast<unsigned int>(rb_num2long(size));
    // Rebuild the atlas at the new size — without this, raylib's atlas
    // stays at LoadFont's default base size and tiny PSDK fonts (size ~11)
    // come out as a downscaled mess.
    rb_Fonts_rebuild_atlas(position);
    return self;
}

VALUE rb_Fonts_define_fill_color(VALUE self, VALUE id, VALUE color)
{
    if (rb_obj_is_kind_of(color, rb_cColor) != Qtrue) {
        rb_raise(rb_eTypeError, "Expected Color got %s.", RSTRING_PTR(rb_class_name(CLASS_OF(color))));
        return self;
    }
    rb_ary_store(rb_ivar_get(rb_mFonts, rb_Fonts_ivFColor), rb_num2long(id), color);
    return self;
}

VALUE rb_Fonts_define_outline_color(VALUE self, VALUE id, VALUE color)
{
    if (rb_obj_is_kind_of(color, rb_cColor) != Qtrue) {
        rb_raise(rb_eTypeError, "Expected Color got %s.", RSTRING_PTR(rb_class_name(CLASS_OF(color))));
        return self;
    }
    rb_ary_store(rb_ivar_get(rb_mFonts, rb_Fonts_ivOColor), rb_num2long(id), color);
    return self;
}

VALUE rb_Fonts_define_shadow_color(VALUE self, VALUE id, VALUE color)
{
    if (rb_obj_is_kind_of(color, rb_cColor) != Qtrue) {
        rb_raise(rb_eTypeError, "Expected Color got %s.", RSTRING_PTR(rb_class_name(CLASS_OF(color))));
        return self;
    }
    rb_ary_store(rb_ivar_get(rb_mFonts, rb_Fonts_ivSColor), rb_num2long(id), color);
    return self;
}

VALUE rb_Fonts_get_default_size(VALUE self, VALUE id)
{
    const std::size_t lid = rb_num2long(id);
    if (lid >= rb_Fonts_Size_Tbl.size()) {
        return LONG2FIX(16);
    }
    return rb_int2inum(static_cast<long>(rb_Fonts_Size_Tbl[lid]));
}

VALUE rb_Fonts_get_fill_color(VALUE self, VALUE id)
{
    return rb_ary_aref(1, &id, rb_ivar_get(rb_mFonts, rb_Fonts_ivFColor));
}

VALUE rb_Fonts_get_outline_color(VALUE self, VALUE id)
{
    return rb_ary_aref(1, &id, rb_ivar_get(rb_mFonts, rb_Fonts_ivOColor));
}

VALUE rb_Fonts_get_shadow_color(VALUE self, VALUE id)
{
    return rb_ary_aref(1, &id, rb_ivar_get(rb_mFonts, rb_Fonts_ivSColor));
}

void Init_Fonts()
{
    rb_mFonts = rb_define_module_under(rb_mLiteRGSS, "Fonts");

    rb_define_module_function(rb_mFonts, "load_font", _rbf rb_Fonts_load_font, 2);
    rb_define_module_function(rb_mFonts, "set_default_size", _rbf rb_Fonts_set_default_size, 2);
    rb_define_module_function(rb_mFonts, "define_fill_color", _rbf rb_Fonts_define_fill_color, 2);
    rb_define_module_function(rb_mFonts, "define_outline_color", _rbf rb_Fonts_define_outline_color, 2);
    rb_define_module_function(rb_mFonts, "define_shadow_color", _rbf rb_Fonts_define_shadow_color, 2);
    rb_define_module_function(rb_mFonts, "get_default_size", _rbf rb_Fonts_get_default_size, 1);
    rb_define_module_function(rb_mFonts, "get_fill_color", _rbf rb_Fonts_get_fill_color, 1);
    rb_define_module_function(rb_mFonts, "get_outline_color", _rbf rb_Fonts_get_outline_color, 1);
    rb_define_module_function(rb_mFonts, "get_shadow_color", _rbf rb_Fonts_get_shadow_color, 1);
    rb_define_module_function(rb_mFonts, "clear_all", _rbf rb_Fonts_clear_all, 0);

    rb_Fonts_ivSize = rb_intern("@default_size");
    rb_Fonts_ivFColor = rb_intern("@fill_color");
    rb_Fonts_ivOColor = rb_intern("@outline_color");
    rb_Fonts_ivSColor = rb_intern("@shadow_color");

    rb_ivar_set(rb_mFonts, rb_Fonts_ivSize, rb_ary_new());
    rb_ivar_set(rb_mFonts, rb_Fonts_ivFColor, rb_ary_new());
    rb_ivar_set(rb_mFonts, rb_Fonts_ivOColor, rb_ary_new());
    rb_ivar_set(rb_mFonts, rb_Fonts_ivSColor, rb_ary_new());
}
