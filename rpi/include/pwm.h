/**
 * @file pwm.h
 * @brief 
 * @date 2022-01-24
 * 
 * @copyright (c) Pierre Boisselier
 * @copyright (c) Guillaume Sanchez
 * 
 */

/* Pour Guillaume :
 * 
 * Quand tu fais une fonction, oublie pas de demander "struct pwm* p" pour pouvoir accéder aux registres.
 * Ex: 
 *     int pwm_output(struct pwm* p, ...) {}
 * Pour accéder aux registres:
 *  p->reg->CTL.USEF2 (pour les structures dans structures)
 * p->reg->DAT1 (pour les variables directements)
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

#define PWM0_BASE_ADDR 0x7e20c000
#define PWM1_BASE_ADDR 0x7e20c800
#define PWM_BLOCK_SIZE 0x28

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct pwm_reg {
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
	} CTL;

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
	} STA;

	struct {
		uint8_t ENAB : 1;
		uint32_t offset : 15;
		uint8_t PANIC : 8;
		uint8_t DREQ : 8;
	} DMAC;

	uint32_t RNG1;
	uint32_t RNG2;

	uint32_t DAT1;
	uint32_t DAT2;

	uint32_t PWM_FIFO;
};
#pragma pack(pop)

struct pwm {
	volatile struct pwm_reg *reg;
};

/**
 * @brief Open a connection to a PWM (PWM0 or PWM1).
 * @param pwm Which PWM to open (0 for PWM0 or 1 for PWM1).
 * @param p Valid pointer to an allocated pwm structure.
 * @return 0 on success, negative value on error. 
 * @todo Better error management and more options.
 */
int pwm_open(int pwm, struct pwm *p)
{
	int mem = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem < 0) {
		perror("unable to open /dev/mem");
		return mem;
	}

	void *map =
		mmap(NULL, PWM_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		     mem, pwm == 0 ? PWM0_BASE_ADDR : PWM1_BASE_ADDR);
	close(mem);

	if (map == MAP_FAILED) {
		perror("unable to mmap the PWM block");
		return -1;
	}

	p->reg = (volatile struct pwm_reg *)map;

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

	munmap(p->reg, PWM_BLOCK_SIZE);
}

#ifdef __cplusplus
}
#endif

#endif // PWM_H