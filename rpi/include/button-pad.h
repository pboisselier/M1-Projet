//////////////////////////////////
/*      Sanchez Guillaume       */
/*      Université de Bordeaux  */
/*      Projet M1 ISE           */
//////////////////////////////////
/* Description :  Mise en oeuvre du button pad 4x4  */
//////////////////////////////////////////////////////////////////////////////
/* Pour un bon fonctionnement du clabier suivre le branchement suivant :    */
/*          PIN GPIO        18  23  24  25      12  16  20  21              */
/*          PIN PAD(lines)  5   6   7   8       4   3   2   1               */
//////////////////////////////////////////////////////////////////////////////
/*  Numbers of pad lines guide :                                            */
/*--------------------------------------------------------------------------*/
/*                       1   2   3   A                                      */
/*                       4   5   6   B                                      */
/*                       7   8   9   C                                      */
/*                       *   0   #   D                                      */
/*                      | | | | | | | |                                     */
/*                      | | | | | | | |                                     */
/*                      1 2 3 4 5 6 7 8                                     */
/*--------------------------------------------------------------------------*/
//////////////////////////////////////////////////////////////////////////////

#ifndef BUTTON_PAD_H
#define BUTTON_PAD_H

#ifndef BUTTONPAD_GPIO_CHIP_DEV
#define BUTTONPAD_GPIO_CHIP_DEV "/dev/gpiochip0"
#endif

// Error defines returned by functions

// No error
#define BUTTONPAD_SUCCESS 0
// Generic error
#define BUTTONPAD_ERR -1
// Bad argument provided to function
#define BUTTONPAD_ERR_ARG -10
// Cannot open the requested GPIO chip
#define BUTTONPAD_ERR_OPEN_CHIP -20
// Cannot open the requested GPIO line
#define BUTTONPAD_ERR_OPEN_LINE -21
// Cannot set a GPIO pin to a specific value
#define BUTTONPAD_ERR_WRITE -30
// Cannot read a GPIO pin value
#define BUTTONPAD_ERR_READ -31

/* LIBRARIES */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include <errno.h>
#include <time.h>
#include <aio.h>
#include <mqueue.h>
#include <sched.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

struct gpiod_line *line1;
struct gpiod_line *line2;
struct gpiod_line *line3;
struct gpiod_line *line4;
struct gpiod_line *line5;
struct gpiod_line *line6;
struct gpiod_line *line7;
struct gpiod_line *line8;

#ifdef __cplusplus
extern "C"
{
#endif

    // Fonction permettant de réserver les lignes GPIO que nous allons utiliser
    int gpio_line_reserve(void)
    {
        struct gpiod_chip *chip = gpiod_chip_open(BUTTONPAD_GPIO_CHIP_DEV);

        if (!chip)
        {
            return BUTTONPAD_ERR_OPEN_CHIP;
        }

        const unsigned int gpio_line1 = 18;
        const unsigned int gpio_line2 = 23;
        const unsigned int gpio_line3 = 24;
        const unsigned int gpio_line4 = 25;
        const unsigned int gpio_line5 = 12;
        const unsigned int gpio_line6 = 16;
        const unsigned int gpio_line7 = 20;
        const unsigned int gpio_line8 = 21;

        line1 = gpiod_chip_get_line(chip, gpio_line1);
        line2 = gpiod_chip_get_line(chip, gpio_line2);
        line3 = gpiod_chip_get_line(chip, gpio_line3);
        line4 = gpiod_chip_get_line(chip, gpio_line4);
        line5 = gpiod_chip_get_line(chip, gpio_line5);
        line6 = gpiod_chip_get_line(chip, gpio_line6);
        line7 = gpiod_chip_get_line(chip, gpio_line7);
        line8 = gpiod_chip_get_line(chip, gpio_line8);

        gpiod_line_request_output(line1, "BUTTON PAD Test", 1);
        gpiod_line_request_output(line2, "BUTTON PAD Test", 1);
        gpiod_line_request_output(line3, "BUTTON PAD Test", 1);
        gpiod_line_request_output(line4, "BUTTON PAD Test", 1);
        gpiod_line_request_input(line5, "BUTTON PAD Test");
        gpiod_line_request_input(line6, "BUTTON PAD Test");
        gpiod_line_request_input(line7, "BUTTON PAD Test");
        gpiod_line_request_input(line8, "BUTTON PAD Test");

        return BUTTONPAD_SUCCESS;
    }

    // Fonction permettant de tester les lignes GPIO et écrire les boutons pressés
    int button_pad_push(void)
    {
        if (!line1 || !line2 || !line3 || !line4 || !line5 || !line6 || !line7 || !line8)
        {
            return BUTTONPAD_ERR_OPEN_LINE;
        }

        while (1)
        {
            if (gpiod_line_get_value(line8) == 1)
            {
                if (gpiod_line_get_value(line8) < 0)
                {
                    return BUTTONPAD_ERR_READ;
                }
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line8);
                gpiod_line_request_input(line1, "BUTTON PAD TEST");
                gpiod_line_request_input(line2, "BUTTON PAD TEST");
                gpiod_line_request_input(line3, "BUTTON PAD TEST");
                gpiod_line_request_input(line4, "BUTTON PAD TEST");
                gpiod_line_request_output(line8, "BUTTON PAD TEST", 1);
                usleep(50000);
                if (gpiod_line_get_value(line1) == 1)
                {
                    printf("1\n");
                }
                else if (gpiod_line_get_value(line2) == 1)
                {
                    printf("2\n");
                }
                else if (gpiod_line_get_value(line3) == 1)
                {
                    printf("3\n");
                }
                else if (gpiod_line_get_value(line4) == 1)
                {
                    printf("A\n");
                }
                usleep(50000);
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line8);
                gpiod_line_request_output(line1, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line2, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line3, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line4, "BUTTON PAD TEST", 1);
                gpiod_line_request_input(line8, "BUTTON PAD TEST");
                usleep(50000);
            }

            if (gpiod_line_get_value(line7) == 1)
            {
                if (gpiod_line_get_value(line7) < 0)
                {
                    return BUTTONPAD_ERR_READ;
                }
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line7);
                gpiod_line_request_input(line1, "BUTTON PAD TEST");
                gpiod_line_request_input(line2, "BUTTON PAD TEST");
                gpiod_line_request_input(line3, "BUTTON PAD TEST");
                gpiod_line_request_input(line4, "BUTTON PAD TEST");
                gpiod_line_request_output(line7, "BUTTON PAD TEST", 1);
                usleep(50000);
                if (gpiod_line_get_value(line1) == 1)
                {
                    printf("4\n");
                }
                else if (gpiod_line_get_value(line2) == 1)
                {
                    printf("5\n");
                }
                else if (gpiod_line_get_value(line3) == 1)
                {
                    printf("6\n");
                }
                else if (gpiod_line_get_value(line4) == 1)
                {
                    printf("B\n");
                }
                usleep(50000);
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line7);
                gpiod_line_request_output(line1, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line2, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line3, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line4, "BUTTON PAD TEST", 1);
                gpiod_line_request_input(line7, "BUTTON PAD TEST");
                usleep(50000);
            }
            if (gpiod_line_get_value(line6) == 1)
            {
                if (gpiod_line_get_value(line6) < 0)
                {
                    return BUTTONPAD_ERR_READ;
                }
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line6);
                gpiod_line_request_input(line1, "BUTTON PAD TEST");
                gpiod_line_request_input(line2, "BUTTON PAD TEST");
                gpiod_line_request_input(line3, "BUTTON PAD TEST");
                gpiod_line_request_input(line4, "BUTTON PAD TEST");
                gpiod_line_request_output(line6, "BUTTON PAD TEST", 1);
                usleep(50000);
                if (gpiod_line_get_value(line1) == 1)
                {
                    printf("7\n");
                }
                else if (gpiod_line_get_value(line2) == 1)
                {
                    printf("8\n");
                }
                else if (gpiod_line_get_value(line3) == 1)
                {
                    printf("9\n");
                }
                else if (gpiod_line_get_value(line4) == 1)
                {
                    printf("C\n");
                }
                usleep(50000);
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line6);
                gpiod_line_request_output(line1, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line2, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line3, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line4, "BUTTON PAD TEST", 1);
                gpiod_line_request_input(line6, "BUTTON PAD TEST");
                usleep(50000);
            }
            if (gpiod_line_get_value(line5) == 1)
            {
                if (gpiod_line_get_value(line5) < 0)
                {
                    return BUTTONPAD_ERR_READ;
                }
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line5);
                gpiod_line_request_input(line1, "BUTTON PAD TEST");
                gpiod_line_request_input(line2, "BUTTON PAD TEST");
                gpiod_line_request_input(line3, "BUTTON PAD TEST");
                gpiod_line_request_input(line4, "BUTTON PAD TEST");
                gpiod_line_request_output(line5, "BUTTON PAD TEST", 1);
                usleep(50000);
                if (gpiod_line_get_value(line1) == 1)
                {
                    printf("*\n");
                }
                else if (gpiod_line_get_value(line2) == 1)
                {
                    printf("0\n");
                }
                else if (gpiod_line_get_value(line3) == 1)
                {
                    printf("#\n");
                }
                else if (gpiod_line_get_value(line4) == 1)
                {
                    printf("D\n");
                }
                usleep(50000);
                gpiod_line_release(line1);
                gpiod_line_release(line2);
                gpiod_line_release(line3);
                gpiod_line_release(line4);
                gpiod_line_release(line5);
                gpiod_line_request_output(line1, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line2, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line3, "BUTTON PAD TEST", 1);
                gpiod_line_request_output(line4, "BUTTON PAD TEST", 1);
                gpiod_line_request_input(line5, "BUTTON PAD TEST");
                usleep(50000);
            }
        }
        return BUTTONPAD_SUCCESS;
    }

#ifdef __cplusplus
}
#endif

#endif // BUTTON_PAD_H
