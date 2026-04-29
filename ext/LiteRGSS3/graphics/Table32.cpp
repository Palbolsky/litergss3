// 1D/2D/3D int32 array. Toplevel `Table32` class — preserves litergss2
// PSDK API. Marshal layout (header + raw int32 heap) is binary-compatible
// with serialized save data.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <cstring>
#include <algorithm>
#include "Table32.h"

VALUE rb_cTable32 = Qnil;

static Table32Data *get_table(VALUE self) { return rb::GetPtr<Table32Data>(self); }

VALUE rb_Table32_initialize(int argc, VALUE *argv, VALUE self)
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
                     "Table32 can be 1D, 2D or 3D but nothing else, requested dimension : %dD", argc);
            return Qnil;
    }
    if (table->header.xsize == 0) table->header.xsize = 1;
    if (table->header.ysize == 0) table->header.ysize = 1;
    if (table->header.zsize == 0) table->header.zsize = 1;
    table->header.dim = argc;
    table->header.data_size = table->header.xsize * table->header.ysize * table->header.zsize;
    delete[] table->heap;
    table->heap = new int32_t[table->header.data_size]();
    return self;
}

VALUE rb_Table32_get(int argc, VALUE *argv, VALUE self)
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

VALUE rb_Table32_set(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    VALUE rx, ry, rz, rv;
    rb_scan_args(argc, argv, "22", &rx, &ry, &rz, &rv);
    unsigned long x = NUM2ULONG(rx);
    unsigned long y, z;
    long v;
    if (NIL_P(rz)) {
        v = RB_NUM2LONG(ry);
        z = y = 0;
    } else if (NIL_P(rv)) {
        v = RB_NUM2LONG(rz);
        y = NUM2ULONG(ry);
        z = 0;
    } else {
        y = NUM2ULONG(ry);
        z = NUM2ULONG(rz);
        v = RB_NUM2LONG(rv);
    }
    if (x >= table->header.xsize || y >= table->header.ysize || z >= table->header.zsize)
        return Qnil;
    table->heap[x + (y * table->header.xsize) +
                (z * table->header.xsize * table->header.ysize)] = static_cast<int32_t>(v);
    return self;
}

VALUE rb_Table32_xSize(VALUE self) { return UINT2NUM(get_table(self)->header.xsize); }
VALUE rb_Table32_ySize(VALUE self) { return UINT2NUM(get_table(self)->header.ysize); }
VALUE rb_Table32_zSize(VALUE self) { return UINT2NUM(get_table(self)->header.zsize); }
VALUE rb_Table32_dim(VALUE self)   { return UINT2NUM(get_table(self)->header.dim); }

static void table_copy_block(int32_t *dheap, int32_t *sheap,
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

VALUE rb_Table32_resize(int argc, VALUE *argv, VALUE self)
{
    auto *table = get_table(self);
    rb_Table32_Struct_Header old_header = table->header;
    int32_t *old_heap = table->heap;
    table->heap = nullptr;

    rb_Table32_initialize(argc, argv, self);

    table_copy_block(table->heap, old_heap,
                     table->header.xsize, table->header.ysize, table->header.zsize,
                     old_header.xsize, old_header.ysize, old_header.zsize);
    delete[] old_heap;
    return self;
}

VALUE rb_Table32_load(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    rb_Table32_Struct_Header header;
    std::memcpy(&header, RSTRING_PTR(str), sizeof(header));
    VALUE arr[3];
    arr[0] = UINT2NUM(header.xsize);
    arr[1] = UINT2NUM(header.ysize);
    arr[2] = UINT2NUM(header.zsize);
    header.dim = cgss::normalize_long(header.dim, 1, 3);
    VALUE rtable = rb_class_new_instance(header.dim, arr, self);
    auto *table = rb::GetPtr<Table32Data>(rtable);
    std::memcpy(table->heap,
                RSTRING_PTR(str) + sizeof(rb_Table32_Struct_Header),
                table->header.data_size * sizeof(int32_t));
    return rtable;
}

VALUE rb_Table32_save(VALUE self, VALUE limit)
{
    auto *table = get_table(self);
    VALUE str1 = rb_str_new(reinterpret_cast<char *>(&table->header),
                            sizeof(rb_Table32_Struct_Header));
    VALUE str2 = rb_str_new(reinterpret_cast<char *>(table->heap),
                            table->header.data_size * sizeof(int32_t));
    return rb_str_concat(str1, str2);
}

VALUE rb_Table32_fill(VALUE self, VALUE val)
{
    auto *table = get_table(self);
    int32_t v = static_cast<int32_t>(RB_NUM2LONG(val));
    for (unsigned int i = 0; i < table->header.data_size; ++i) table->heap[i] = v;
    return self;
}

void Init_Table32()
{
    rb_cTable32 = rb_define_class("Table32", rb_cObject);
    rb_define_alloc_func(rb_cTable32, rb::Alloc<Table32Data>);
    rb_define_method(rb_cTable32, "initialize", _rbf rb_Table32_initialize, -1);
    rb_define_method(rb_cTable32, "[]", _rbf rb_Table32_get, -1);
    rb_define_method(rb_cTable32, "[]=", _rbf rb_Table32_set, -1);
    rb_define_method(rb_cTable32, "xsize", _rbf rb_Table32_xSize, 0);
    rb_define_method(rb_cTable32, "ysize", _rbf rb_Table32_ySize, 0);
    rb_define_method(rb_cTable32, "zsize", _rbf rb_Table32_zSize, 0);
    rb_define_method(rb_cTable32, "dim", _rbf rb_Table32_dim, 0);
    rb_define_method(rb_cTable32, "resize", _rbf rb_Table32_resize, -1);
    rb_define_method(rb_cTable32, "fill", _rbf rb_Table32_fill, 1);
    rb_define_method(rb_cTable32, "_dump", _rbf rb_Table32_save, 1);
    rb_define_singleton_method(rb_cTable32, "_load", _rbf rb_Table32_load, 1);
}
