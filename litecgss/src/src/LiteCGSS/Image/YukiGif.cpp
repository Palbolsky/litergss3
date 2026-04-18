/* Include cstddef first, otherwise libnsgif.hpp won't "compile" */
#include <cstddef>
#include <libnsgif.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "YukiGif.h"

#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

double& cgss::YukiGif::FrameDelta() {
	static double frameDelta = 16.666666666666666;
	return frameDelta;
}

//--- LibNgif util functions ---
static void* bitmap_create(int width, int height) {
	/* ensure a stupidly large bitmap is not created */
	if (static_cast<long long>(width) * static_cast<long long>(height) >
			(MAX_IMAGE_BYTES / BYTES_PER_PIXEL)) {
		return NULL;
	}
	return calloc(width * height, BYTES_PER_PIXEL);
}

static void bitmap_set_opaque(void *, bool) {}
static bool bitmap_test_opaque(void *) { return false; }
static void bitmap_modified(void *) {}
static unsigned char *bitmap_get_buffer(void *bitmap) {
	return reinterpret_cast<unsigned char*>(bitmap);
}
static void bitmap_destroy(void * bitmap) {
	free(bitmap);
}

static gif_bitmap_callback_vt bitmap_callbacks = {
	bitmap_create,
	bitmap_destroy,
	bitmap_get_buffer,
	bitmap_set_opaque,
	bitmap_test_opaque,
	bitmap_modified
};

cgss::YukiGif::YukiGif() :
	m_gif(std::make_unique<gif_animation>()) {
	gif_create(m_gif.get(), &bitmap_callbacks);
}

bool cgss::YukiGif::load(const char* rawMemory, std::size_t size) {
	const auto code = gif_initialise(m_gif.get(), size, reinterpret_cast<unsigned char*>(const_cast<char*>(rawMemory)));
	return code == GIF_OK || code == GIF_WORKING;
}

cgss::YukiGif::~YukiGif() {
	gif_finalise(m_gif.get());
}

void cgss::YukiGif::drawOn(sf::Texture& texture) const {
	texture.update(reinterpret_cast<sf::Uint8*>(m_gif->frame_image), m_gif->width, m_gif->height, 0, 0);
}

bool cgss::YukiGif::update(sf::Texture& texture) {
	if (((m_gif->frames[m_frame].frame_delay * 10) <= m_counter) ||
		(m_frame == 0 && m_counter == 0)) {
		if (m_frame != 0 || m_counter != 0) {
			m_counter -= m_gif->frames[m_frame].frame_delay * 10;
			m_frame++;
			if (m_frame >= m_gif->frame_count) {
				m_frame = 0;
			}
		}
		if (gif_decode_frame(m_gif.get(), m_frame) != GIF_OK) {
			return false;
		}

		drawOn(texture);
	}
	m_counter += FrameDelta();
	return true;
}

unsigned int cgss::YukiGif::width() const {
	return m_gif->width;
}

unsigned int cgss::YukiGif::height() const {
	return m_gif->height;
}

unsigned long cgss::YukiGif::frame() const {
	return m_frame;
}

void cgss::YukiGif::setFrame(unsigned long frame) {
	m_frame = frame;
}

unsigned long cgss::YukiGif::frameCount() const {
	return m_gif->frame_count;
}
