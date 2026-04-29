// 1D/2D/3D short integer array. Class lives at the top level (Table)
// rather than under LiteRGSS::Table — preserves litergss2 PSDK API.
//
// The on-disk Marshal layout (header struct + raw int16 heap, byte-for-byte)
// must stay binary-compatible with serialized PSDK save data.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <cstring>
#include "Table.h"

VALUE rb_cTable = Qnil;

static TableData *get_table(VALUE self) { return rb::GetPtr<TableData>(self); }

static void copy_line(short *&dst, short *&src, long ini_x, long max_x)
{
    short *src_end = src + (max_x - ini_x);
    for (; src < src_end; ++src) { *dst = *src; ++dst; }
}

static void copy_modulo_y_part(short *dst, short *yheap2, long ox2,
                               long target_x, long offsetx,
                               long src_xsize, long m)
{
    short *xheap2 = yheap2 + ox2;
    long target_x2 = target_x - offsetx;
    if (target_x2 > (src_xsize - ox2)) target_x2 = (src_xsize - ox2);
    copy_line(dst, xheap2, 0, target_x2);

    for (long i = 0; i < m; ++i) {
        xheap2 = yheap2;
        copy_line(dst, xheap2, 0, src_xsize);
    }

    xheap2 = yheap2;
    target_x2 = target_x - offsetx + ox2 - (m + 1) * src_xsize;
    if (target_x2 > src_xsize) target_x2 = src_xsize;
    copy_line(dst, xheap2, 0, target_x2);
}

VALUE rb_Table_initialize(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    switch (argc) {
        case 1:
            table->header.xsize = NUM2ULONG(argv[0]);
            table->header.ysize = 1;
            table->header.zsize = 1;
            break;
        case 2:
            table->header.xsize = NUM2ULONG(argv[0]);
            table->header.ysize = NUM2ULONG(argv[1]);
            table->header.zsize = 1;
            break;
        case 3:
            table->header.xsize = NUM2ULONG(argv[0]);
            table->header.ysize = NUM2ULONG(argv[1]);
            table->header.zsize = NUM2ULONG(argv[2]);
            break;
        default:
            rb_raise(rb_eRGSSError,
                     "Table can be 1D, 2D or 3D but nothing else, requested dimension : %dD", argc);
            return Qnil;
    }
    if (table->header.xsize == 0) table->header.xsize = 1;
    if (table->header.ysize == 0) table->header.ysize = 1;
    if (table->header.zsize == 0) table->header.zsize = 1;
    table->header.dim = argc;
    table->header.data_size = table->header.xsize * table->header.ysize * table->header.zsize;
    delete[] table->heap;
    table->heap = new short[table->header.data_size]();
    return self;
}

VALUE rb_Table_get(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    VALUE rx, ry, rz;
    rb_scan_args(argc, argv, "12", &rx, &ry, &rz);
    unsigned long x = NUM2ULONG(rx);
    unsigned long y = NIL_P(ry) ? 0 : NUM2ULONG(ry);
    unsigned long z = NIL_P(rz) ? 0 : NUM2ULONG(rz);
    if (x >= table->header.xsize || y >= table->header.ysize || z >= table->header.zsize)
        return Qnil;
    return INT2NUM(table->heap[x + (y * table->header.xsize) +
                               (z * table->header.xsize * table->header.ysize)]);
}

VALUE rb_Table_set(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    VALUE rx, ry, rz, rv;
    rb_scan_args(argc, argv, "22", &rx, &ry, &rz, &rv);
    unsigned long x = NUM2ULONG(rx);
    unsigned long y, z;
    short v;
    if (NIL_P(rz)) {
        v = NUM2SHORT(ry);
        z = y = 0;
    } else if (NIL_P(rv)) {
        v = NUM2SHORT(rz);
        y = NUM2ULONG(ry);
        z = 0;
    } else {
        y = NUM2ULONG(ry);
        z = NUM2ULONG(rz);
        v = NUM2SHORT(rv);
    }
    if (x >= table->header.xsize || y >= table->header.ysize || z >= table->header.zsize)
        return Qnil;
    table->heap[x + (y * table->header.xsize) +
                (z * table->header.xsize * table->header.ysize)] = v;
    return self;
}

VALUE rb_Table_xSize(VALUE self) { return UINT2NUM(get_table(self)->header.xsize); }
VALUE rb_Table_ySize(VALUE self) { return UINT2NUM(get_table(self)->header.ysize); }
VALUE rb_Table_zSize(VALUE self) { return UINT2NUM(get_table(self)->header.zsize); }
VALUE rb_Table_dim(VALUE self)   { return UINT2NUM(get_table(self)->header.dim); }

static void table_copy_block(short *dheap, short *sheap,
                             unsigned long dxsize, unsigned long dysize, unsigned long dzsize,
                             unsigned long sxsize, unsigned long sysize, unsigned long szsize)
{
    unsigned long xsize = dxsize < sxsize ? dxsize : sxsize;
    unsigned long ysize = dysize < sysize ? dysize : sysize;
    unsigned long zsize = dzsize < szsize ? dzsize : szsize;
    for (unsigned long z = 0; z < zsize; ++z) {
        unsigned long doz = z * dxsize * dysize;
        unsigned long soz = z * sxsize * sysize;
        for (unsigned long y = 0; y < ysize; ++y) {
            unsigned long doy = doz + (y * dxsize);
            unsigned long soy = soz + (y * sxsize);
            for (unsigned long x = 0; x < xsize; ++x)
                dheap[doy++] = sheap[soy++];
        }
    }
}

VALUE rb_Table_resize(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    // Snapshot the current heap + dims, then re-init in place; copy old
    // contents into the new heap before releasing the snapshot's buffer.
    rb_Table_Struct_Header old_header = table->header;
    short *old_heap = table->heap;
    table->heap = nullptr;  // detach so initialize won't free the old buffer

    rb_Table_initialize(argc, argv, self);

    table_copy_block(table->heap, old_heap,
                     table->header.xsize, table->header.ysize, table->header.zsize,
                     old_header.xsize, old_header.ysize, old_header.zsize);
    delete[] old_heap;
    return self;
}

VALUE rb_Table_load(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    rb_Table_Struct_Header header;
    std::memcpy(&header, RSTRING_PTR(str), sizeof(header));
    VALUE arr[3];
    arr[0] = UINT2NUM(header.xsize);
    arr[1] = UINT2NUM(header.ysize);
    arr[2] = UINT2NUM(header.zsize);
    header.dim = cgss::normalize_long(header.dim, 1, 3);
    VALUE rtable = rb_class_new_instance(header.dim, arr, self);
    auto *table = rb::GetPtr<TableData>(rtable);
    std::memcpy(table->heap,
                RSTRING_PTR(str) + sizeof(rb_Table_Struct_Header),
                table->header.data_size * sizeof(short));
    return rtable;
}

VALUE rb_Table_save(VALUE self, VALUE limit)
{
    auto *table = get_table(self);
    VALUE str1 = rb_str_new(reinterpret_cast<char *>(&table->header),
                            sizeof(rb_Table_Struct_Header));
    VALUE str2 = rb_str_new(reinterpret_cast<char *>(table->heap),
                            table->header.data_size * sizeof(short));
    return rb_str_concat(str1, str2);
}

VALUE rb_Table_fill(VALUE self, VALUE val)
{
    auto *table = get_table(self);
    short v = NUM2SHORT(val);
    for (unsigned int i = 0; i < table->header.data_size; ++i) table->heap[i] = v;
    return self;
}

VALUE rb_Table_copy(VALUE self, VALUE source, VALUE dest_offset_x, VALUE dest_offset_y)
{
    auto *table = get_table(self);
    auto *src = rb::GetSafeOrNull<TableData>(source, rb_cTable);
    if (src == nullptr) {
        rb_raise(rb_eRGSSError, "Tables require having another Table as first parameter of copy method.");
        return Qfalse;
    }
    long offsetx = NUM2LONG(dest_offset_x);
    long offsety = NUM2LONG(dest_offset_y);
    if (offsetx < 0 || static_cast<std::size_t>(offsetx) >= table->header.xsize) return Qfalse;
    if (offsety < 0 || static_cast<std::size_t>(offsety) >= table->header.ysize) return Qfalse;

    std::size_t target_z = std::min<std::size_t>(table->header.zsize, src->header.zsize);
    std::size_t target_x = std::min<std::size_t>(table->header.xsize, offsetx + src->header.xsize);
    std::size_t target_y = std::min<std::size_t>(table->header.ysize, offsety + src->header.ysize);

    std::size_t deltay = table->header.xsize;
    std::size_t deltaz = deltay * table->header.ysize;
    std::size_t deltay2 = src->header.xsize;
    std::size_t deltaz2 = deltay2 * src->header.ysize;

    short *zheap1 = table->heap + (offsety * deltay + offsetx);
    short *zheap2 = src->heap;

    for (std::size_t z = 0; z < target_z; ++z) {
        short *yheap1 = zheap1;
        short *yheap2 = zheap2;
        for (std::size_t y = static_cast<std::size_t>(offsety); y < target_y; ++y) {
            short *xheap1 = yheap1;
            short *xheap2 = yheap2;
            copy_line(xheap1, xheap2, offsetx, target_x);
            yheap1 += deltay;
            yheap2 += deltay2;
        }
        zheap1 += deltaz;
        zheap2 += deltaz2;
    }
    return Qtrue;
}

VALUE rb_Table_copy_modulo(VALUE self, VALUE source,
                           VALUE source_origin_x, VALUE source_origin_y,
                           VALUE dest_offset_x, VALUE dest_offset_y,
                           VALUE dest_width, VALUE dest_height)
{
    auto *table = get_table(self);
    auto *src = rb::GetSafeOrNull<TableData>(source, rb_cTable);
    if (src == nullptr) {
        rb_raise(rb_eRGSSError, "Tables require having another Table as first parameter of copy_modulo method.");
        return Qfalse;
    }
    long offsetx = NUM2LONG(dest_offset_x);
    long offsety = NUM2LONG(dest_offset_y);
    long ox2 = NUM2LONG(source_origin_x);
    long oy2 = NUM2LONG(source_origin_y);
    if (offsetx < 0 || static_cast<std::size_t>(offsetx) >= table->header.xsize) return Qfalse;
    if (offsety < 0 || static_cast<std::size_t>(offsety) >= table->header.ysize) return Qfalse;
    if (ox2 < 0 || static_cast<std::size_t>(ox2) >= src->header.xsize) return Qfalse;
    if (oy2 < 0 || static_cast<std::size_t>(oy2) >= src->header.ysize) return Qfalse;

    std::size_t src_xsize = src->header.xsize;
    std::size_t src_ysize = src->header.ysize;

    std::size_t target_z = std::min<std::size_t>(table->header.zsize, src->header.zsize);
    std::size_t target_x = std::min<std::size_t>(table->header.xsize, offsetx + NUM2LONG(dest_width));
    std::size_t target_y = std::min<std::size_t>(table->header.ysize, offsety + NUM2LONG(dest_height));

    long n = (target_y - offsety - src_ysize + oy2) / src_ysize;
    long m = (target_x - offsetx - src_xsize + ox2) / src_xsize;
    if (n < 0) n = 0;
    if (m < 0) m = 0;

    long deltay = table->header.xsize;
    long deltaz = deltay * table->header.ysize;
    long deltay2 = src->header.xsize;
    long deltaz2 = deltay2 * src->header.ysize;

    short *zheap1 = table->heap + (offsety * deltay + offsetx);
    short *zheap2 = src->heap;

    for (std::size_t z = 0; z < target_z; ++z) {
        short *yheap1 = zheap1;
        short *yheap2 = zheap2 + oy2 * deltay2;
        long target_y2 = target_y - offsety;
        if (target_y2 > static_cast<long>(src_ysize - oy2)) target_y2 = (src_ysize - oy2);

        for (long y = 0; y < target_y2; ++y) {
            copy_modulo_y_part(yheap1, yheap2, ox2, target_x, offsetx, src_xsize, m);
            yheap1 += deltay;
            yheap2 += deltay2;
        }

        for (long j = 0; j < n; ++j) {
            yheap2 = zheap2;
            for (long y = 0; y < static_cast<long>(src_ysize); ++y) {
                copy_modulo_y_part(yheap1, yheap2, ox2, target_x, offsetx, src_xsize, m);
                yheap1 += deltay;
                yheap2 += deltay2;
            }
        }

        yheap2 = zheap2;
        target_y2 = target_y - offsety + oy2 - (n + 1) * src_ysize;
        if (target_y2 > static_cast<long>(src_ysize)) target_y2 = static_cast<long>(src_ysize);
        for (long y = 0; y < target_y2; ++y) {
            copy_modulo_y_part(yheap1, yheap2, ox2, target_x, offsetx, src_xsize, m);
            yheap1 += deltay;
            yheap2 += deltay2;
        }

        zheap1 += deltaz;
        zheap2 += deltaz2;
    }
    return Qtrue;
}

void Init_Table()
{
    rb_cTable = rb_define_class("Table", rb_cObject);
    rb_define_alloc_func(rb_cTable, rb::Alloc<TableData>);
    rb_define_method(rb_cTable, "initialize", _rbf rb_Table_initialize, -1);
    rb_define_method(rb_cTable, "[]", _rbf rb_Table_get, -1);
    rb_define_method(rb_cTable, "[]=", _rbf rb_Table_set, -1);
    rb_define_method(rb_cTable, "xsize", _rbf rb_Table_xSize, 0);
    rb_define_method(rb_cTable, "ysize", _rbf rb_Table_ySize, 0);
    rb_define_method(rb_cTable, "zsize", _rbf rb_Table_zSize, 0);
    rb_define_method(rb_cTable, "dim", _rbf rb_Table_dim, 0);
    rb_define_method(rb_cTable, "resize", _rbf rb_Table_resize, -1);
    rb_define_method(rb_cTable, "fill", _rbf rb_Table_fill, 1);
    rb_define_method(rb_cTable, "copy", _rbf rb_Table_copy, 3);
    rb_define_method(rb_cTable, "copy_modulo", _rbf rb_Table_copy_modulo, 7);
    rb_define_method(rb_cTable, "_dump", _rbf rb_Table_save, 1);
    rb_define_singleton_method(rb_cTable, "_load", _rbf rb_Table_load, 1);
}
