#ifndef LCM2004A_I2C__h
#define LCM2004A_I2C__h

#include <Arduino.h>
#include <Wire.h>
#include <jm_LCM2004A_I2C.h>
#include <SDDPArduinoVendor.h>

class LCM2004A_SDDPDisplayInterface : public SDDPDisplayInterface 
{
public:
  LCM2004A_SDDPDisplayInterface(uint8_t i2cAddress) : i2cAddress(i2cAddress) {}

  LCM2004A_SDDPDisplayInterface(const LCM2004A_SDDPDisplayInterface &) = delete;
  LCM2004A_SDDPDisplayInterface &operator=(const LCM2004A_SDDPDisplayInterface &) = delete;
  LCM2004A_SDDPDisplayInterface(const LCM2004A_SDDPDisplayInterface &&) = delete;
  LCM2004A_SDDPDisplayInterface &operator=(const LCM2004A_SDDPDisplayInterface &&) = delete;

  ~LCM2004A_SDDPDisplayInterface()
  {
    if (display) {
      delete display;
    }
  }

  jm_LCM2004A_I2C *rawDisplay()
  {
    return display;
  }

  // #pragma mark SDDPDisplayInterface methods

  void initialize()
  {
    display = new jm_LCM2004A_I2C(i2cAddress);

    Wire.begin();
    display->begin();
  }

  void consumerEstablished(String consumerId, String onChannel)
  {
    display->clear();
    display->setCursor(0, 0);
  }

  virtual void clear(void)
  {
    display->clear();
  }

  virtual void home(void)
  {
    display->home();
  }

  virtual void writeAt(int column, int row, String toWrite)
  {
    display->setCursor(column, row);
    display->print(toWrite);
  }

  virtual void toggleDisplay(bool on)
  {
    on ? display->display() : display->noDisplay();
  }

  virtual void toggleCursor(bool on)
  {
    on ? display->cursor() : display->noCursor();
  }

  virtual void toggleCursorBlink(bool on)
  {
    on ? display->blink() : display->noBlink();
  }
  
  virtual void scrollDisplay(ScrollDir direction)
  {
    switch (direction) {
      case Left:
      display->scrollDisplayLeft();
      break;

      case Right:
      display->scrollDisplayRight();
      break;

      default: break;
    }
  }

private:
  uint8_t i2cAddress;
  jm_LCM2004A_I2C *display;
};

#endif