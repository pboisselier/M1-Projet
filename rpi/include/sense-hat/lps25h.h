/**
 * @brief Library for the LPS25H, a pressure sensor from STM
 * @file lps25h.h
 * @date 2022-01-04
 * 
 * @copyright (c) Pierre Boisselier 
 * 
 * @details 
 * More information about the sensor itself can be found here: https://www.st.com/en/mems-and-sensors/lps25h.html 
 * More information about the I2C device driver: https://www.kernel.org/doc/html/v5.4/i2c/dev-interface.html
 * 
 * 24-bits precisions pressure sensor.
 * 
 * ## Specifics to the Sense-Hat
 * 
 * - Pin SDO/SA0 is tied to 0V, the LSB of the I2C slave address is 0.
 * - Pin INT is tied to a pad, not wired to a pin
 */

#ifndef LPS25H_H
#define LPS25H_H

/**
 * @name I2C default settings
 * @{
 */

#ifndef LPS25H_I2C_ADDR
/** @brief Default I2C slave address on the Sense-Hat */
#define LPS25H_I2C_ADDR 0x5C
#endif
#ifndef RPI_I2C_DEVICE
/** @brief Default I2C device for the Raspberry Pi */
#define RPI_I2C_DEVICE "/dev/i2c-1"
#endif

/**
 * @} 
 * @name LPS25H register addresses
 * @{
 */

#define LPS25H_REG_REF_PXL 0x08
#define LPS25H_REG_REF_PL 0x09
#define LPS25H_REG_REF_PH 0x0A
#define LPS25H_REG_WHOAMI 0x0F
#define LPS25H_REG_RES_CONF 0x10
#define LPS25H_REG_CTRL_R1 0x20
#define LPS25H_REG_CTRL_R2 0x21
#define LPS25H_REG_CTRL_R3 0x22
#define LPS25H_REG_CTRL_R4 0x23
#define LPS25H_REG_INT_CFG 0x24
#define LPS25H_REG_INT_SRC 0x25
#define LPS25H_REG_STATUS 0x27
/** @brief Pressure value, first 8bits */
#define LPS25H_REG_PRESS_OUTXL 0x27
/** @brief Pressure value, second 8bits */
#define LPS25H_REG_PRESS_OUTL 0x29
/** @brief Pressure value, last 8bits */
#define LPS25H_REG_PRESS_OUTH 0x2A
/** @brief Temperature value, first 8 bits */
#define LPS25H_REG_TEMP_OUTL 0x2B
/** @brief Temperature value, last 8 bits */
#define LPS25H_REG_TEMP_OUTH 0x2C
#define LPS25H_REG_FIFO_CTRL 0x2E
#define LPS25H_REG_FIFO_STATUS 0x2F
/** @brief Threshold for pressure interrupt (LOW part) */
#define LPS25H_REG_THS_PL 0x30
/** @brief Threshold for pressure interrupt (HIGH part) */
#define LPS25H_REG_THS_PH 0x30
#define LPS25H_REG_RPDSL 0x39
#define LPS25H_REG_RPDSH 0x39

/**
 * @}
 * @name LPS25H register controls.
 * @{
 */

/** @brief Power down control, default = 0. */
#define LPS25H_CTRL1_PD 0x80
/** @brief Output data rate, default = 0. */
#define LPS25H_CTRL1_ODR2 0x40
/** @brief Output data rate, default = 0. */
#define LPS25H_CTRL1_ODR1 0x20
/** @brief Output data rate, default = 0. */
#define LPS25H_CRTL1_ODR0 0x10
/** @brief Interrupt circuit enable, default = 0 (disabled). */
#define LPS25H_CTRL1_DIFFEN 0x08
/** @brief Block data update, default = 0 (continuous). */
#define LPS25H_CTRL1_BDU 0x04
/** @brief Reset AutoZero function with default reference values. */
#define LPS25H_CTRL1_RESETAZ 0x02
/** @brief SPI Mode selection, default = 0 (4-wire) */
#define LPS25H_CTRL1_SIM 0x01

/** @brief Reboot memory content. */
#define LPS25H_CTRL2_BOOT 0x80
/** @brief Enable FIFO, default = 0. */
#define LPS25H_CTRL2_FIFOEN 0x40
/** @brief Enable FIFO watermak level, default = 0. */
#define LPS25H_CTRL2_WTMEN 0x20
/** @brief Enable 1Hz ODR decimation. */
#define LPS25H_CRTL2_FIFOMEAN 0x10
/** @brief Software reset. */
#define LPS25H_CTRL2_SWRST 0x04
/** @brief Enable AutoZero, default = 0 (disabled). 
 * Setting to 1 will use the current pressure and temperature as
 * new reference.
*/
#define LPS25H_CTRL2_AUTOZERO 0x02
/** @brief Enable one shot, default = 0 (waiting) 
 * Writing 1 will trigger a single measurement of pressure and temperature.
 * After that measurement, this will be reset to 0, data will
 * be available in the output registers.
*/
#define LPS25H_CTRL2_ONESHOT 0x01

/** @brief Intterupt active high or low, default = 0 (active high). */
#define LPS25H_CTRL3_INTHL 0x80
/** @brief Push-pull or open drain on interrupt pad, default = 0 (push-pull). */
#define LPS25H_CTRL3_PPOD 0x40
/** @brief Type of interrupt, see Table 19 in official datasheet. */
#define LPS25H_CTRL3_INT1S2 0x02
/** @brief Type of interrupt, see Table 19 in official datasheet. */
#define LPS25H_CTRL3_INT1S1 0x01

/**
 * @}
 */

/** @brief Resolution of the pressure output. */
#define LSP25H_PRES_RESOLUTION 24
/** @brief Resolution of the temperature output. */
#define LPS25H_TEMP_RESOLUTION 16
/** @brief Pressure sensor LSB. */
#define LPS25H_PRESS_LSB 4096.0
/** @brief Temperature sensor LSB. */
#define LPS25H_TEMP_LSB 480.0
/** @brief Temperature sensor constant. */
#define LPS25H_TEMP_CONSTANT 42.5

/**
 * @name Error values returned by functions.
 * @{
 */

/** @brief Generic error. */
#define LPS25H_ERR -1
/** @brief Bad argument provided to function. */
#define LPS25H_ERR_ARG -10
/** @brief I2C device not opened. */
#define LPS25H_ERR_NOPEN -11
/** @brief Cannot read from I2C device. */
#define LPS25H_ERR_READ -20
/** @brief Connot write to I2C device. */
#define LPS25H_ERR_WRITE -21

/**
 * @}
 * @name Optional flags.
 * @{
 */

/** @brief Shutdown and wakeup only when reading the pressure. */
#define LPS25H_OPT_WAKEUP 0x01
///** @brief Enable FIFO mode, read 32 values at one time. */
//#define LPS25H_OPT_FIFO 0x10
///** @brief Enable FIFO mode, read mean value of 32 values. */
//#define LPS25H_OPT_FIFO_MEAN 0x20
///** @brief Enable FIFO mode, continuously fill the 32 value buffer. */
//#define LPS25H_OPT_FIFO_STREAM 0x40

/**
 * @}
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <linux/i2c-dev.h>

#ifdef __cplusplus
extern "C" {
#endif

// IDEAS
// Use FIFO mode (32 pressures stored register)?
// Factory calib, maybe provide a way to calibrate?
// ALlows for interrupt on something else than the PI?

struct lps25h {
	int i2c_fd;
	///< I2C device file descriptor.
	int options;
	///< Optional flags used.
	uint8_t ctrl_r1;
	///< Configuration register 1.
	uint8_t ctrl_r2;
	///< Configuration register 2.
	uint8_t ctrl_r3;
	///< Configuration register 3.
	//int32_t fifo_pressure[32];
	//int32_t fifo_temperature[32];
};

/**
 * @brief Write byte to I2C device.
 * @param fd I2C file descriptor.
 * @param byte Byte to write.
 * @return Resulting write call. 
 */
static inline int write_byte(int fd, uint8_t reg_addr, uint8_t byte)
{
	uint8_t buf[2] = { reg_addr, byte };
	return write(fd, buf, sizeof(buf));
}

/**
 * @brief Read a register from the I2C device.
 * @param fd I2C file descriptor.
 * @param reg_addr Register address.
 * @return Register value.
 */
static inline uint8_t read_register(int fd, uint8_t reg_addr)
{
	if (write(fd, &reg_addr, sizeof(reg_addr)) < 0)
		return -1;
	read(fd, &reg_addr, sizeof(reg_addr));
	return reg_addr;
}

/**
 * @brief Return the 2s' complement value.
 * @param value Value to convert.
 * @param nbits Number of bits.
 * @return 2s' complement. 
 */
static inline int32_t complement_2s(const int32_t value, const int32_t nbits)
{
	int32_t val = value;
	if (value & ((uint32_t)1 << (nbits - 1)))
		val = (value) - ((uint32_t)1 << nbits);
	return val;
}

/**
 * @brief Change current power regime, ON or OFF.
 * @param fd I2C file descriptor.
 * @param ctrl_r1 Current value of the control register R1, prevents overwriting other settings.
 * @param power_on 1 = power on device, 0 = power off device.
 * @return 0 on success, negative value on error. 
 */
static int change_power_status(int fd, uint8_t ctrl_r1, int power_on)
{
	if (power_on) {
		ctrl_r1 |= LPS25H_CTRL1_PD;
	} else {
		ctrl_r1 ^= LPS25H_CTRL1_PD;
	}

	return write_byte(fd, LPS25H_REG_CTRL_R1, LPS25H_CTRL1_PD | ctrl_r1);
}

/**
 * @brief Open connection to the LPS25H pressure sensor.
 * @param lps Allocated structure that will serve as access.
 * @param i2c_device I2C device path.
 * @param slave_addr LPS25H slave address.
 * @param flags Optional flags.
 * @return 0 on success, negative value on failure.
 */
int lps25h_init_c_l(struct lps25h *lps, const char *i2c_device,
		    const long slave_addr, const int options)
{
	if (!lps || !i2c_device) {
		return LPS25H_ERR_ARG;
	}

	lps->options = options;
	lps->i2c_fd = -1;

	int fd = open(i2c_device, O_RDWR);
	if (fd < 0)
		return LPS25H_ERR_NOPEN;

	if (ioctl(fd, I2C_SLAVE, slave_addr) < 0) {
		(void)close(fd);
		return LPS25H_ERR_NOPEN;
	}

	uint8_t conf_reg1 = LPS25H_CTRL1_BDU;

	/* Power up the sensor at the start */
	if ((options & LPS25H_OPT_WAKEUP) == 0) {
		conf_reg1 |= LPS25H_CTRL1_PD;
	}

	/* Configure sensor */
	if (write_byte(fd, LPS25H_REG_CTRL_R1, conf_reg1) < 0) {
		int err = errno;
		(void)close(fd);
		errno = err;
		return LPS25H_ERR_WRITE;
	}

	lps->i2c_fd = fd;
	lps->ctrl_r1 = conf_reg1;
	lps->ctrl_r2 = 0x0;
	lps->ctrl_r3 = 0x0;

	return 0;
}

/**
 * @brief Open connection to the LPS25H pressure sensor with default values.
 * @param lps Allocated structure that will serve as access.
 * @return File descriptor on success, or negative value on failure.
 */
int lps25h_init(struct lps25h *lps)
{
	return lps25h_init_c_l(lps, RPI_I2C_DEVICE, LPS25H_I2C_ADDR, 0);
}

/**
 * @brief Close a connection to the LPS25H.
 * @param lps Connection to the LPS25H.
 * @return 0 on success, negative value on failure. 
 */
int lps25h_close(struct lps25h *lps)
{
	if (!lps) {
		return LPS25H_ERR_ARG;
	}

	if (lps->i2c_fd < 0) {
		return LPS25H_ERR_NOPEN;
	}

	if (close(lps->i2c_fd) < 0) {
		return LPS25H_ERR;
	}

	return 0;
}

/**
 * @brief Read pressure value from sensor.
 * @param i2c_fd Connection to the LPS25H.
 * @return Pressure value, 24bits, negative value on failure.
 */
double lps25h_get_pressure(const struct lps25h *lps)
{
	/*
	 * From official datasheets:
	 * Pressure output data: Pout(hPa) = PRESS_OUT / 4096
	 * Example: P_OUT = 0x3ED000 LSB = 4116480 LSB = 4116480/4096 hPa= 1005 hPa
	 * Default value is 0x2F800 = 760 hP
	 */

	if (!lps || lps->i2c_fd < 0) {
		return LPS25H_ERR_NOPEN;
	}

	/* Power on sensor. */
	if (lps->options & LPS25H_OPT_WAKEUP) {
		change_power_status(lps->i2c_fd, lps->ctrl_r1, 1);
	}

	/* Request a conversion. */
	if (write_byte(lps->i2c_fd, LPS25H_REG_CTRL_R2,
		       LPS25H_CTRL2_ONESHOT | lps->ctrl_r2) < 0) {
		return LPS25H_ERR_WRITE;
	}

	/* 
	 * Wait for the conversion to be finished.
	 * This waits for the LPS25H_CTRL2_ONESHOT bit to be 0 again.
	 * WARNING: This way spams the I2C bus.
	 */
	while (read_register(lps->i2c_fd, LPS25H_REG_CTRL_R2) ^ lps->ctrl_r2)
		;

	/* Read conversion data. */
	uint8_t raw_pressure[3];
	raw_pressure[0] = read_register(lps->i2c_fd, LPS25H_REG_PRESS_OUTH);
	raw_pressure[1] = read_register(lps->i2c_fd, LPS25H_REG_PRESS_OUTL);
	raw_pressure[2] = read_register(lps->i2c_fd, LPS25H_REG_PRESS_OUTXL);

	int32_t pressure =
		raw_pressure[0] << 16 | raw_pressure[1] << 8 | raw_pressure[2];

	/* Power off device. */
	if (lps->options & LPS25H_OPT_WAKEUP) {
		change_power_status(lps->i2c_fd, lps->ctrl_r1, 0);
	}

	return (pressure / LPS25H_PRESS_LSB);
}

///**
// * @brief Read content of the 32 values FIFO buffer.
// * @param lps Connection to the LPS25H.
// * @param 
// * @return 0 on success, negative value on failure.
// * @warning FIFO mode needs to be enabled with the optional flags.
// */
//int lps25h_read_pressure_multiple(const struct lps25h *lps, int32_t buf[32]);

/**
 * @brief Read current temperature value from sensor.
 * @param lps Connection to the sensor.
 * @return Temperature. 
 */
double lps25h_get_temperature(const struct lps25h *lps)
{
	/*
	 * From official datasheets:
	 * Temperature data are expressed as TEMP_OUT_H & TEMP_OUT_L as 2’s
         * complement numbers. Temperature output data:
	 * T(°C) = 42.5 + (TEMP_OUT / 480)
         * If TEMP_OUT = 0 LSB then Temperature is 42.5 °C
	 */
	if (!lps || lps->i2c_fd < 0) {
		return LPS25H_ERR_NOPEN;
	}

	/* Power on sensor. */
	if (lps->options & LPS25H_OPT_WAKEUP) {
		change_power_status(lps->i2c_fd, lps->ctrl_r1, 1);
	}

	/* Request a conversion. */
	if (write_byte(lps->i2c_fd, LPS25H_REG_CTRL_R2,
		       LPS25H_CTRL2_ONESHOT | lps->ctrl_r2) < 0) {
		return LPS25H_ERR_WRITE;
	}

	/* 
	 * Wait for the conversion to be finished.
	 * This waits for the LPS25H_CTRL2_ONESHOT bit to be 0 again.
	 * WARNING: This way spams the I2C bus.
	 */
	while (read_register(lps->i2c_fd, LPS25H_REG_CTRL_R2) ^ lps->ctrl_r2)
		;

	/* Read conversion data. */
	uint8_t raw_temperature[2];
	raw_temperature[0] = read_register(lps->i2c_fd, LPS25H_REG_TEMP_OUTH);
	raw_temperature[1] = read_register(lps->i2c_fd, LPS25H_REG_TEMP_OUTL);

	int16_t temperature = raw_temperature[0] << 8 | raw_temperature[1];

	/* Power off device. */
	if (lps->options & LPS25H_OPT_WAKEUP) {
		change_power_status(lps->i2c_fd, lps->ctrl_r1, 0);
	}

	return LPS25H_TEMP_CONSTANT + (temperature / LPS25H_TEMP_LSB);
}

#ifdef __cplusplus
}
#endif

#endif // LPS25H_H