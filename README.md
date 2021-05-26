Arduino vendor library for the [Simple Device Drawing Protocol](https://sddp.electricsheep.co)

## Usage

### Overview

By defining concrete implementations of the [`SDDPDisplayInterface`](SDDPArduinoVendor.h#L11-L35), it's simple to build support for different concrete drawing APIs.

### API

_TBD_

## Example: `LCM2004A_I2C`

Included in this library are a [fully-functional concrete implementation of `SDDPDisplayInterface` for the `LCM2004A` displays connected via the I<sup>2</sup>C bus](interfaces/LCM2004A_I2C.h) (via the [`jm_LCM2004A_I2C`](https://github.com/jmparatte/jm_LCM2004A_I2C) library) and a corresponding consumer application for driving said displays of this type (type-specific in this case only because it draws initialization UI) in [`examples/LCM2004A_I2C_driver.ino`](examples/LCM2004A_I2C_driver.ino).