#include <stdio.h>
#include <stdlib.h>
#include <sense-hat/led-matrix.h>

int main(int argc, char *argv[])
{
	struct led_matrix *matrix = led_matrix_open("/dev/fb0");
	if (!matrix) {
		perror("unable to open led matrix");
		return EXIT_FAILURE;
	}

	int r = atoi(argv[1]);
	int g = atoi(argv[2]);
	int b = atoi(argv[3]);
	pixel_t color = { { r, g, b } };
        color_t c = {r, g, b};
	printf("pixel_t size: %d\nuint16_t size: %d\nrgb size: %d\nraw size: %d\n",
	       sizeof(color), sizeof(uint16_t), sizeof(color.rgb),
	       sizeof(color.raw));
	printf("raw: %x\n", color.raw);
	//led_matrix_raw_fill(matrix, color);
        led_matrix_fill(matrix, c);


	//color.raw = 0x3003;
	//for (unsigned i = 0; i < LEDMATRIX_WIDTH; ++i) {
	//	for (unsigned j = 0; j < LEDMATRIX_HEIGHT; ++j) {
	//		led_matrix_raw_set(matrix, j, i, color);
	//	}
	//}

	led_matrix_close(matrix);
}