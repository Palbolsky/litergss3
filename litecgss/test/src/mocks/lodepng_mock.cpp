#include <stdlib.h>

unsigned lodepng_decode32(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize) {
  static const auto bpp = 32;
  *w = 10;
  *h = 10;
  const size_t n = (*w) * (*h);
  const auto size = ((n / 8) * bpp) + ((n & 7) * bpp + 7) / 8;
  *out = static_cast<unsigned char*>(malloc(size));
  return 0;
}

unsigned lodepng_decode32_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename) {
  return lodepng_decode32(out, w, h, NULL, 0);
}

unsigned lodepng_encode32_file(const char* filename, const unsigned char* image, unsigned w, unsigned h) {
  return 0;
}

unsigned lodepng_encode32(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h) {
  return 0;
}

const char* lodepng_error_text(unsigned code) {
  return "";
}
