#ifndef LODEPNG_H
#define LODEPNG_H

unsigned lodepng_decode32_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename);
unsigned lodepng_encode32_file(const char* filename, const unsigned char* image, unsigned w, unsigned h);
unsigned lodepng_decode32(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize);
const char* lodepng_error_text(unsigned code);
unsigned lodepng_encode32(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h);
#endif
