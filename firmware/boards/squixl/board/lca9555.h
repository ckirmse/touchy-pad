// SPDX-License-Identifier: GPL-3.0-or-later
//
// LCA9555 16-bit I2C IO expander driver.
// Register-compatible with NXP PCA9555; I2C address is hardware-selectable.

#pragma once

#include <cstdint>
#include "driver/i2c_master.h"

class Lca9555 {
public:
    Lca9555();

    // Attach to an already-initialised I2C master bus.
    // Returns true if the device acknowledges at addr.
    bool init(i2c_master_bus_handle_t bus, uint8_t addr = 0x20);

    // Configure a pin as input (true) or output (false) with initial output value.
    void pinMode(uint8_t pin, bool input, bool initial_value = false);

    // Set an output pin high (true) or low (false).
    void write(uint8_t pin, bool value);

    // Read an input pin.
    bool read(uint8_t pin);

private:
    i2c_master_dev_handle_t m_dev;
    uint8_t m_output[2];
    uint8_t m_config[2];

    uint8_t readReg(uint8_t reg);
    void writeReg(uint8_t reg, uint8_t value);
};
