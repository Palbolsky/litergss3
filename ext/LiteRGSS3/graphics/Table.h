#ifndef Table_H
#define Table_H

#include "RubyValue.h"

extern VALUE rb_cTable;
void Init_Table();

struct rb_Table_Struct_Header
{
    unsigned int dim = 0u;
    unsigned int xsize = 0u;
    unsigned int ysize = 0u;
    unsigned int zsize = 0u;
    unsigned int data_size = 0u;
};

struct TableData
{
    rb_Table_Struct_Header header{};
    short *heap = nullptr;

    ~TableData()
    {
        delete[] heap;
        heap = nullptr;
    }
};

#endif
