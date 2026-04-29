#ifndef Table32_H
#define Table32_H

#include <cstdint>
#include "RubyValue.h"

extern VALUE rb_cTable32;
void Init_Table32();

struct rb_Table32_Struct_Header
{
    unsigned int dim = 0u;
    unsigned int xsize = 0u;
    unsigned int ysize = 0u;
    unsigned int zsize = 0u;
    unsigned int data_size = 0u;
};

struct Table32Data
{
    rb_Table32_Struct_Header header{};
    int32_t *heap = nullptr;

    ~Table32Data()
    {
        delete[] heap;
        heap = nullptr;
    }
};

#endif
