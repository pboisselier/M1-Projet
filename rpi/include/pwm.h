/**
 * @file pwm.h
 * @brief 
 * @date 2022-01-24
 * 
 * @copyright (c) Pierre Boisselier
 * 
 */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

#define PWM0 0
#define PWM1 1

/** @brief Raspberry Pi 4 Base address for peripherals. */
#define BCM2711_BASE 0xfe000000
//#define BCM2711_CLK_PASSWD 0x5A
#define BCM2711_CLK_PASSWD 0x5A000000

#define PWM0_BASE_ADDR 0x20c000
#define GPIO_BASE_ADDR 0x200000
#define GPIO_GPFSEL1_OFFSET (0x04 / 4)
#define GPIO_CLK_BASE_ADDR 0x101000
//#define GPIO_CLK2_OFFSET (0x80 / 4)
#define GPIO_CLK2_OFFSET 40
#define GPIO_FSEL_ALT1 0b100
#define GPIO_FSEL_DEFAULT 0b000

#define BCM_PAGE_SIZE 4 * 1024

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t arm_base_addr = 0xfe00000000;

#pragma pack(push, 1)
struct pwm_reg {
	/* 0x00 */
	union {
		struct {
			uint16_t offset : 16;
			uint8_t MSEN2 : 1;
			uint8_t reserved : 1;
			uint8_t USEF2 : 1;
			uint8_t POLA2 : 1;
			uint8_t SBIT2 : 1;
			uint8_t RPTL2 : 1;
			uint8_t MODE2 : 1;
			uint8_t PWEN2 : 1;
			uint8_t MSEN1 : 1;
			uint8_t CLRF : 1;
			uint8_t USEF1 : 1;
			uint8_t POLA1 : 1;
			uint8_t SBIT1 : 1;
			uint8_t RPTL1 : 1;
			uint8_t MODE1 : 1;
			uint8_t PWEN1 : 1;
		} reg;
		uint32_t raw;
	} CTL;
	/* 0x04 */
	union {
		struct {
			uint32_t offset : 20;
			uint8_t STA2 : 1;
			uint8_t STA1 : 1;
			uint8_t BERR : 1;
			uint8_t reserved : 2;
			uint8_t GAPO2 : 1;
			uint8_t GAPO1 : 1;
			uint8_t RERRR : 1;
			uint8_t WERR1 : 1;
			uint8_t EMPT1 : 1;
			uint8_t FULL1 : 1;
		} reg;
		uint32_t raw;
	} STA;
	/* 0x08 */
	union {
		struct {
			uint8_t ENAB : 1;
			uint32_t offset : 15;
			uint8_t PANIC : 8;
			uint8_t DREQ : 8;
		} reg;
		uint32_t raw;
	} DMAC;

	/* 0x10 */
	uint32_t RNG1;
	/* 0x14 */
	uint32_t DAT1;
	/* 0x18 */
	uint32_t PWM_FIFO;
	/* 0x20 */
	uint32_t RNG2;
	/* 0x24 */
	uint32_t DAT2;
};
struct clk_reg {
	union {
		struct {
			uint8_t PASSWD : 8;
			uint16_t reserved1 : 13;
			uint8_t MASH : 2;
			uint8_t FLIP : 1;
			uint8_t BUSY : 1;
			uint8_t reserved2 : 1;
			uint8_t KILL : 1;
			uint8_t ENAB : 1;
			uint8_t SRC : 4;
		} reg;
		uint32_t raw;
	} CTL;

	union {
		struct {
			uint8_t PASSWD : 8;
			uint16_t DIVI : 12;
			uint16_t DIVF : 12;
		} reg;
		uint32_t raw;
	} DIV;
};
struct gpio_fsel_reg {
	uint8_t reserved : 1;
	uint8_t FSEL19 : 3;
	uint8_t FSEL18 : 3;
	uint8_t FSEL17 : 3;
	uint8_t FSEL16 : 3;
	uint8_t FSEL15 : 3;
	uint8_t FSEL14 : 3;
	uint8_t FSEL13 : 3;
	uint8_t FSEL12 : 3;
	uint8_t FSEL11 : 3;
	uint8_t FSEL10 : 3;
};
#pragma pack(pop)

struct pwm {
	volatile struct pwm_reg *pwm0;
	volatile struct clk_reg *clk;
	volatile struct gpio_fsel_reg *gpio;
};

int pwm_test(struct pwm *p)
{
	p->pwm0->RNG1 = 1024;
	//p->clk->CTL.reg.PASSWD = BCM2711_CLK_PASSWD;
	//p->clk->CTL.reg.SRC = 0x01;
	p->clk->CTL.raw = BCM2711_CLK_PASSWD | 0x01;

	//while (p->clk->CTL.reg.BUSY)
	while ((p->clk->CTL.raw & 0x80))
		printf("Clock BUSY: %08x\n", p->clk->CTL.raw);

	///p->clk->DIV.reg.PASSWD = BCM2711_CLK_PASSWD;
	///p->clk->DIV.reg.DIVI = 0x5a;
	p->clk->DIV.raw = BCM2711_CLK_PASSWD | (0x5A << 12);

	//p->clk->CTL.reg.PASSWD = BCM2711_CLK_PASSWD;
	//p->clk->CTL.reg.ENAB = 1;
	//p->clk->CTL.reg.SRC = 1;
	p->clk->CTL.raw = BCM2711_CLK_PASSWD | 0x11;

	p->pwm0->CTL.PWEN1 = 1;
	p->pwm0->DAT1 = 0xf0f0f0f0;

	printf("GPIO: %08x\n", p->gpio);
	printf("CLK CTL: %08x\tDIV: %08x\n", p->clk->CTL.raw, p->clk->DIV.raw);
	printf("PWM CTL: %08x\n", p->pwm0);

	return 0;
}

/**
 * @brief 
 * @param p Valid pointer to an opened pwm structure.
 * @param channel Channel 0 or 1 (GPIO Pin 12 or 13).
 * @return 0 on success, -1 on error.
 */
int pwm_enable_channel(struct pwm *p, int channel)
{
	if (!p || !p->gpio) {
		return -1;
	}

	switch (channel) {
	case 0:
		p->gpio->FSEL12 = GPIO_FSEL_ALT1;
		break;
	case 1:
		p->gpio->FSEL13 = GPIO_FSEL_ALT1;
		break;
	default:
		return -1;
	}
}

/**
 * @brief Open a connection to the PWM0 module.
 * @param p Valid pointer to an allocated pwm structure.
 * @return 0 on success, negative value on error. 
 * @todo Better error management and more options.
 * 
 * Only the PWM0 is accessible on the Raspberry Pi 4, the other PWM is connected
 * to GPIO lines 40 and +, which are not wired to the GPIO header. 
 */
int pwm_open(struct pwm *p)
{
	int mem = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem < 0) {
		perror("unable to open /dev/mem");
		return mem;
	}

	void *map = mmap(NULL, BCM_PAGE_SIZE, PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem, BCM2711_BASE | PWM0_BASE_ADDR);
	if (map == MAP_FAILED) {
		perror("unable to mmap the PWM block");
		return -1;
	}

	p->pwm0 = (volatile struct pwm_reg *)map;

	map = mmap(NULL, BCM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem,
		   BCM2711_BASE | GPIO_BASE_ADDR);
	if (map == MAP_FAILED) {
		perror("unable to mmap the PWM block");
		return -1;
	}

	p->gpio = (volatile struct gpio_fsel_reg *)(map + GPIO_GPFSEL1_OFFSET);

	map = mmap(NULL, BCM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem,
		   BCM2711_BASE | GPIO_CLK_BASE_ADDR);
	if (map == MAP_FAILED) {
		perror("unable to mmap the PWM block");
		return -1;
	}

	p->clk = (volatile struct clk_reg *)(map + GPIO_CLK2_OFFSET);

	close(mem);

	return 0;
}

/**
 * @brief Close connection to the PWM.
 * @param p Valid pointer to a pwm structure.
 */
void pwm_close(struct pwm *p)
{
	if (!p) {
		return;
	}

	munmap(p->pwm0, BCM_PAGE_SIZE);
	munmap(p->gpio, BCM_PAGE_SIZE);
	munmap(p->clk, BCM_PAGE_SIZE);
}

#ifdef __cplusplus
}
#endif

#endif // PWM_H