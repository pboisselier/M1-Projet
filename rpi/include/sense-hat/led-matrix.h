/**
 * @brief Library for the 8x8 LED Matrix on the Sense-Hat
 * 
 * @file led-matrix.h
 * @ingroup Sense-Hat
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-30
 * 
 * @details
 * This libary allows one to use the 8x8 LED Matrix attached to the Raspberry Pi Sense-Hat.  
 * On the Raspberry Pi the LED Matrix is recognized as a Framebuffer with the `rpisense-fb.ko` driver.  
 * 
 * 
 */

#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LEDMATRIX_O_EXCLUSIVE 1
/** @brief Default framebuffer for the Led Matrix */
#define LEDMATRIX_DEV_FB "/dev/fb0"

/** @brief Led Matrix led height */
#define LEDMATRIX_HEIGHT 8
/** @brief Led Matrix led width */
#define LEDMATRIX_WIDTH 8
/** @brief Led Matrix byte size 
 */
#define LEDMATRIX_SZ (LEDMATRIX_WIDTH * LEDMATRIX_HEIGHT * sizeof(pixel_t))

/**
 * @brief Matrix Pixel format, BGR565.
 * Packing for serialization.
 */
#pragma pack(push, 1)
typedef union {
	struct {
		uint8_t b : 5;
		uint8_t g : 6;
		uint8_t r : 5;
	} rgb;
	uint16_t raw;
} pixel_t;
#pragma pack(pop)

/**
 * @brief Color in 24-bit pixel format, the well-known one.
 */
typedef struct color {
	uint8_t red;
	///< Red channel.
	uint8_t green;
	///< Green channel.
	uint8_t blue;
	///< Blue channel.
} color_t;

/**
 * @brief Led matrix structure.
 */
struct led_matrix {
	const char *dev;
	///< Framebuffer device file.
	pixel_t *map;
	///< Framebuffer map.
	int flags;
	///< Optional flags.
};

/** @brief Look-up table for fast conversion. */
static const uint8_t rgb5_rgb8[32] = { 0,   8,	 16,  24,  32,	41,  49,  57,
				       65,  74,	 82,  90,  98,	106, 115, 123,
				       131, 139, 148, 156, 164, 172, 180, 189,
				       197, 205, 213, 222, 230, 238, 246, 255 };

/** @brief Look-up table for fast conversion. */
static const uint8_t rgb6_rgb8[64] = {
	0,   4,	  8,   12,  16,	 20,  24,  28,	32,  36,  40,  44,  48,
	52,  56,  60,  64,  68,	 72,  76,  80,	85,  89,  93,  97,  101,
	105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153,
	157, 161, 165, 170, 174, 178, 182, 186, 190, 194, 198, 202, 206,
	210, 214, 218, 222, 226, 230, 234, 238, 242, 246, 250, 255
};

/** @brief Default error when returning a pixel_t. */
static const pixel_t px_err = (pixel_t)((uint16_t)UINT16_MAX);

/** @brief Convert (x,y) to map index. */
static inline size_t _get_index(unsigned x, unsigned y)
{
	return (x % LEDMATRIX_WIDTH) +
	       (y % LEDMATRIX_HEIGHT) * LEDMATRIX_HEIGHT;
}

/**
 * @brief Convert from a color_t (RGB888) to pixel_t (BGR565).
 * @param color Color to convert.
 * @return Conversion result. 
 */
pixel_t led_matrix_color_to_pixel(const color_t color)
{
	pixel_t px;
	px.rgb.r = color.red >> 3;
	px.rgb.g = color.green >> 2;
	px.rgb.b = color.blue >> 3;
	return px;
}

/**
 * @brief Convert from a pixel_t (BGR565) to color_t (RGB888).
 * @param pixel Pixel to convert.
 * @return Conversion result. 
 */
color_t led_matrix_pixel_to_color(const pixel_t pixel)
{
	color_t c;
	/* Use a look-up table to speed up computation */
	c.red = rgb5_rgb8[pixel.rgb.r];
	c.green = rgb6_rgb8[pixel.rgb.g];
	c.blue = rgb5_rgb8[pixel.rgb.b];
	return c;
}
/**
 * @brief Open the 8x8 LED Matrix.
 * @param fb_dev Path to the framebuffer device (default is /dev/fb0).
 * @return Pointer to a led_matrix object.
 */
struct led_matrix *led_matrix_open(const char *fb_dev)
{
	int err = 0;
	int fd = open(fb_dev, O_RDWR);
	if (fd < 0)
		return NULL;

	struct led_matrix *matrix = malloc(sizeof(*matrix));
	if (!matrix)
		goto cleanup1;

	matrix->dev = fb_dev;

	/* Map the framebuffer into memory */
	matrix->map = (pixel_t *)mmap(
		NULL, LEDMATRIX_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (matrix->map == MAP_FAILED)
		goto cleanup2;

	/* We can close after mapping, see man for mmap */
	close(fd);

	return matrix;

cleanup2:
	err = err == 0 ? errno : err;
	free(matrix);
cleanup1:
	err = err == 0 ? errno : err;
	close(fd);
	errno = err;
	return NULL;
}

/**
 * @brief Close the 8x8 LED Matrix and frees resources.
 * @param matrix Pointer to a led_matrix object.
 * @return 0 on success, -1 on failure.
 */
int led_matrix_close(struct led_matrix *matrix)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}

	if (munmap(matrix->map, LEDMATRIX_SZ) < 0)
		return -1;

	free(matrix);

	return 0;
}

/**
 * @brief Fill the whole matrix with a static color (pixel_t format).
 * @param matrix Pointer to a led_matrix object.
 * @param color_raw Color to fill the matrix with.
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_raw_fill(struct led_matrix *matrix, const pixel_t color_raw)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}

	for (size_t i = 0; i < LEDMATRIX_HEIGHT * LEDMATRIX_WIDTH; ++i) {
		matrix->map[i] = color_raw;
	}

	return 0;
}

/**
 * @brief Change a pixel in the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param x X coordinate (will wrap if over the width).
 * @param y Y coordinate (will wrap if over the height).
 * @param pixel New value for the pixel in BGR565 format.
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_raw_set(struct led_matrix *matrix, unsigned x, unsigned y,
		       const pixel_t pixel)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}

	matrix->map[_get_index(x, y)] = pixel;
	return 0;
}

/**
 * @brief Get the value of a pixel in the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param x X coordinate (will wrap if over the width).
 * @param y Y coordinate (will wrap if over the height).
 * @return The value of the pixel in the BGR565 format or UINT16_MAX.
 */
const pixel_t led_matrix_raw_get(struct led_matrix *matrix, unsigned x,
				 unsigned y)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return px_err;
	}

	return matrix->map[_get_index(x, y)];
}

/**
 * @brief Change a pixel in the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param x X coordinate (will wrap if over the width).
 * @param y Y coordinate (will wrap if over the height).
 * @param color New value for the pixel in RGB888 format. 
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_set(struct led_matrix *matrix, unsigned x, unsigned y,
		   const color_t color)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}

	matrix->map[_get_index(x, y)] = led_matrix_color_to_pixel(color);
	return 0;
}

/**
 * @brief Get the value of a pixel in the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param x X coordinate (will wrap if over the width).
 * @param y Y coordinate (will wrap if over the height).
 * @return The value of the pixel in the RGB888 format.
 * @warning This function is not checking for errors!
 */
const color_t led_matrix_get(struct led_matrix *matrix, unsigned x, unsigned y)
{
	pixel_t px = led_matrix_raw_get(matrix, x, y);
	return led_matrix_pixel_to_color(px);
}

/**
 * @brief Fill the whole matrix with a static color (RGB888 format).
 * @param matrix Pointer to a led_matrix object.
 * @param color Color to fill the matrix with.
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_fill(struct led_matrix *matrix, const color_t color)
{
	return led_matrix_raw_fill(matrix, led_matrix_color_to_pixel(color));
}

/**
 * @brief Get a copy of the matrix content in BGR565 format.
 * @param matrix Pointer to a led_matrix object.
 * @return Array of LEDMATRIX_HEIGH * LEDMATRIX_WIDTH with the matrix content, NULL on failure.
 */
pixel_t *led_matrix_raw_screenshot(struct led_matrix *matrix)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return NULL;
	}

	pixel_t *array = malloc(LEDMATRIX_SZ);
	if (!array)
		return NULL;

	memcpy(array, matrix->map, LEDMATRIX_SZ);
	return array;
}

/**
 * @brief Display an array in BGR565 format on the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param px_array Array of pixel to display on the matrix.
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_raw_display(struct led_matrix *matrix, const pixel_t *px_array)
{
	if (!matrix || !matrix->map || !px_array) {
		errno = EINVAL;
		return -1;
	}

	memcpy(matrix->map, px_array, LEDMATRIX_SZ);
	return 0;
}

/**
 * @brief Get a copy of the matrix content in RGB888 format.
 * @param matrix Pointer to a led_matrix object.
 * @return Array of LEDMATRIX_HEIGH * LEDMATRIX_WIDTH with the matrix content, NULL on failure.
 * @todo Should freeze the display if there's an animation?
 */
color_t *led_matrix_screenshot(struct led_matrix *matrix)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return NULL;
	}
	color_t *array =
		malloc(sizeof(*array) * LEDMATRIX_HEIGHT * LEDMATRIX_WIDTH);
	if (!array)
		return NULL;

	for (size_t i = 0; i < LEDMATRIX_WIDTH * LEDMATRIX_HEIGHT; ++i) {
		array[i] = led_matrix_pixel_to_color(matrix->map[i]);
	}

	return array;
}

/**
 * @brief Display an array in RGB888 format on the matrix.
 * @param matrix Pointer to a led_matrix object.
 * @param px_array Array of pixel to display on the matrix.
 * @return 0 on success, -1 on failure. 
 */
int led_matrix_display(struct led_matrix *matrix, const color_t *px_array)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}
	for (size_t i = 0; i < LEDMATRIX_WIDTH * LEDMATRIX_HEIGHT; ++i) {
		matrix->map[i] = led_matrix_color_to_pixel(px_array[i]);
	}

	return 0;
}

/**
 * @brief Rotate the content on the display.
 * @param matrix Pointer to a led_matrix object.
 * @param angle Angle in degrees.
 * @return 0 on sucess, -1 on failure. 
 * @todo Implement it.
 */
int led_matrix_rotate(struct led_matrix *matrix, int angle)
{
	if (!matrix || !matrix->map) {
		errno = EINVAL;
		return -1;
	}

	angle = angle % 360;

	if (angle % 90 == 0) {
	}

	return 0;
}

//int led_matrix_move(struct led_matrix *matrix, int delta_x, int delta_y, color_t *filler);

#ifdef __cplusplus
}
#endif

#endif // LED_MATRIX_H