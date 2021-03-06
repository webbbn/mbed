/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include "mbed_assert.h"
#include "i2c_api.h"

#if DEVICE_I2C

#include "cmsis.h"
#include "pinmap.h"

/* Timeout values for flags and events waiting loops. These timeouts are
   not based on accurate values, they just guarantee that the application will
   not remain stuck if the I2C communication is corrupted. */
#define FLAG_TIMEOUT ((int)0x4000)
#define LONG_TIMEOUT ((int)0x8000)

static const PinMap PinMap_I2C_SDA[] = {
    {PA_14, I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {PB_7,  I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {PB_9,  I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {NC,    NC,    0}
};

static const PinMap PinMap_I2C_SCL[] = {
    {PA_15, I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {PB_6,  I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {PB_8,  I2C_1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_NOPULL, GPIO_AF4_I2C1)},
    {NC,    NC,    0}
};

I2C_HandleTypeDef I2cHandle;

void i2c_init(i2c_t *obj, PinName sda, PinName scl)
{
    // Determine the I2C to use
    I2CName i2c_sda = (I2CName)pinmap_peripheral(sda, PinMap_I2C_SDA);
    I2CName i2c_scl = (I2CName)pinmap_peripheral(scl, PinMap_I2C_SCL);

    obj->i2c = (I2CName)pinmap_merge(i2c_sda, i2c_scl);
    MBED_ASSERT(obj->i2c != (I2CName)NC);

    // Enable I2C clock
    __I2C1_CLK_ENABLE();

    // Configure the I2C clock source
    __HAL_RCC_I2C1_CONFIG(RCC_I2C1CLKSOURCE_SYSCLK);
  
    // Configure I2C pins
    pinmap_pinout(sda, PinMap_I2C_SDA);
    pinmap_pinout(scl, PinMap_I2C_SCL);
    pin_mode(sda, OpenDrain);
    pin_mode(scl, OpenDrain);

    // Reset to clear pending flags if any
    i2c_reset(obj);

    // I2C configuration
    i2c_frequency(obj, 100000); // 100 kHz per default
}

void i2c_frequency(i2c_t *obj, int hz)
{
    uint32_t tim = 0;

    MBED_ASSERT((hz == 100000) || (hz == 400000) || (hz == 1000000));

    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);

    // Update the SystemCoreClock variable.
    SystemCoreClockUpdate();

    /*
       Values calculated with I2C_Timing_Configuration_V1.0.1.xls file (see AN4235)
       * Standard mode (up to 100 kHz)
       * Fast Mode (up to 400 kHz)
       * Fast Mode Plus (up to 1 MHz)
       Below values obtained with:
       - I2C clock source = 64 MHz (System Clock w/ HSI) or 72 (System Clock w/ HSE)
       - Analog filter delay = ON
       - Digital filter coefficient = 0
    */
    if (SystemCoreClock == 64000000) {
        switch (hz) {
            case 100000:
                tim = 0x10B17DB4; // Standard mode with Rise time = 120ns, Fall time = 120ns
                break;
            case 400000:
                tim = 0x00E22163; // Fast Mode with Rise time = 120ns, Fall time = 120ns
                break;
            case 1000000:
                tim = 0x00A00D1E; // Fast Mode Plus with Rise time = 120ns, Fall time = 10ns
                // Enable the Fast Mode Plus capability
                __HAL_SYSCFG_FASTMODEPLUS_ENABLE(HAL_SYSCFG_FASTMODEPLUS_I2C1);
                break;
            default:
                break;
        }
    } else if (SystemCoreClock == 72000000) {
        switch (hz) {
            case 100000:
                tim = 0x10D28DCB; // Standard mode with Rise time = 120ns, Fall time = 120ns
                break;
            case 400000:
                tim = 0x00F32571; // Fast Mode with Rise time = 120ns, Fall time = 120ns
                break;
            case 1000000:
                tim = 0x00C00D24; // Fast Mode Plus with Rise time = 120ns, Fall time = 10ns
                // Enable the Fast Mode Plus capability
                __HAL_SYSCFG_FASTMODEPLUS_ENABLE(HAL_SYSCFG_FASTMODEPLUS_I2C1);
                break;
            default:
                break;
        }
    }

    // I2C configuration
    I2cHandle.Init.Timing           = tim;
    I2cHandle.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    I2cHandle.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLED;
    I2cHandle.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLED;
    I2cHandle.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLED;
    I2cHandle.Init.OwnAddress1      = 0;
    I2cHandle.Init.OwnAddress2      = 0;
    I2cHandle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    HAL_I2C_Init(&I2cHandle);
}

inline int i2c_start(i2c_t *obj)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    int timeout;

    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);

    // Clear Acknowledge failure flag
    __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_AF);

    // Generate the START condition
    i2c->CR2 |= I2C_CR2_START;

    // Wait the START condition has been correctly sent
    timeout = FLAG_TIMEOUT;
    while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_BUSY) == RESET) {
        if ((timeout--) == 0) {
            return 1;
        }
    }

    return 0;
}

inline int i2c_stop(i2c_t *obj)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);

    // Generate the STOP condition
    i2c->CR2 |= I2C_CR2_STOP;

    return 0;
}

int i2c_read(i2c_t *obj, int address, char *data, int length, int stop)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);
    int timeout;
    int count;
    int value;

    /* update CR2 register */
    i2c->CR2 = (i2c->CR2 & (uint32_t)~((uint32_t)(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP)))
               | (uint32_t)(((uint32_t)address & I2C_CR2_SADD) | (((uint32_t)length << 16) & I2C_CR2_NBYTES) | (uint32_t)I2C_SOFTEND_MODE | (uint32_t)I2C_GENERATE_START_READ);

    // Read all bytes
    for (count = 0; count < length; count++) {
        value = i2c_byte_read(obj, 0);
        data[count] = (char)value;
    }

    // Wait transfer complete
    timeout = LONG_TIMEOUT;
    while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_TC) == RESET) {
        timeout--;
        if (timeout == 0) {
            return -1;
        }
    }

    __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_TC);

    // If not repeated start, send stop.
    if (stop) {
        i2c_stop(obj);
        /* Wait until STOPF flag is set */
        timeout = FLAG_TIMEOUT;
        while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_STOPF) == RESET) {
            timeout--;
            if (timeout == 0) {
                return -1;
            }
        }
        /* Clear STOP Flag */
        __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_STOPF);
    }

    return length;
}

int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);
    int timeout;
    int count;

    /* update CR2 register */
    i2c->CR2 = (i2c->CR2 & (uint32_t)~((uint32_t)(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP)))
               | (uint32_t)(((uint32_t)address & I2C_CR2_SADD) | (((uint32_t)length << 16) & I2C_CR2_NBYTES) | (uint32_t)I2C_SOFTEND_MODE | (uint32_t)I2C_GENERATE_START_WRITE);

    for (count = 0; count < length; count++) {
        i2c_byte_write(obj, data[count]);
    }

    // Wait transfer complete
    timeout = FLAG_TIMEOUT;
    while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_TC) == RESET) {
        timeout--;
        if (timeout == 0) {
            return -1;
        }
    }

    __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_TC);

    // If not repeated start, send stop.
    if (stop) {
        i2c_stop(obj);
        /* Wait until STOPF flag is set */
        timeout = FLAG_TIMEOUT;
        while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_STOPF) == RESET) {
            timeout--;
            if (timeout == 0) {
                return -1;
            }
        }
        /* Clear STOP Flag */
        __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_STOPF);
    }

    return count;
}

int i2c_byte_read(i2c_t *obj, int last)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    int timeout;

    // Wait until the byte is received
    timeout = FLAG_TIMEOUT;
    while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_RXNE) == RESET) {
        if ((timeout--) == 0) {
            return -1;
        }
    }

    return (int)i2c->RXDR;
}

int i2c_byte_write(i2c_t *obj, int data)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    int timeout;

    // Wait until the previous byte is transmitted
    timeout = FLAG_TIMEOUT;
    while (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_TXIS) == RESET) {
        if ((timeout--) == 0) {
            return 0;
        }
    }

    i2c->TXDR = (uint8_t)data;

    return 1;
}

void i2c_reset(i2c_t *obj)
{
    __I2C1_FORCE_RESET();
    __I2C1_RELEASE_RESET();
}

#if DEVICE_I2CSLAVE

void i2c_slave_address(i2c_t *obj, int idx, uint32_t address, uint32_t mask)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    uint16_t tmpreg;

    // disable
    i2c->OAR1 &= (uint32_t)(~I2C_OAR1_OA1EN);
    // Get the old register value
    tmpreg = i2c->OAR1;
    // Reset address bits
    tmpreg &= 0xFC00;
    // Set new address
    tmpreg |= (uint16_t)((uint16_t)address & (uint16_t)0x00FE); // 7-bits
    // Store the new register value
    i2c->OAR1 = tmpreg;
    // enable
    i2c->OAR1 |= I2C_OAR1_OA1EN;
}

void i2c_slave_mode(i2c_t *obj, int enable_slave)
{

    I2C_TypeDef *i2c = (I2C_TypeDef *)(obj->i2c);
    uint16_t tmpreg;

    // Get the old register value
    tmpreg = i2c->OAR1;

    // Enable / disable slave
    if (enable_slave == 1) {
        tmpreg |= I2C_OAR1_OA1EN;
    } else {
        tmpreg &= (uint32_t)(~I2C_OAR1_OA1EN);
    }

    // Set new mode
    i2c->OAR1 = tmpreg;

}

// See I2CSlave.h
#define NoData         0 // the slave has not been addressed
#define ReadAddressed  1 // the master has requested a read from this slave (slave = transmitter)
#define WriteGeneral   2 // the master is writing to all slave
#define WriteAddressed 3 // the master is writing to this slave (slave = receiver)

int i2c_slave_receive(i2c_t *obj)
{
    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);
    int retValue = NoData;

    if (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_BUSY) == 1) {
        if (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_ADDR) == 1) {
            if (__HAL_I2C_GET_FLAG(&I2cHandle, I2C_FLAG_DIR) == 1)
                retValue = ReadAddressed;
            else
                retValue = WriteAddressed;
            __HAL_I2C_CLEAR_FLAG(&I2cHandle, I2C_FLAG_ADDR);
        }
    }

    return (retValue);
}

int i2c_slave_read(i2c_t *obj, char *data, int length)
{
    char size = 0;

    while (size < length) data[size++] = (char)i2c_byte_read(obj, 0);

    return size;
}

int i2c_slave_write(i2c_t *obj, const char *data, int length)
{
    char size = 0;
    I2cHandle.Instance = (I2C_TypeDef *)(obj->i2c);

    do {
        i2c_byte_write(obj, data[size]);
        size++;
    } while (size < length);

    return size;
}


#endif // DEVICE_I2CSLAVE

#endif // DEVICE_I2C
