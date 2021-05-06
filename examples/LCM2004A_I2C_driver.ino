#include <SDDPArduinoVendor.h>
#include <interfaces/LCM2004A_I2C.h>

// must define: VERSION (string), HOSTNAME (string, must be unique across the deployment),
// WIFI_SSID (string), WIFI_PASSWORD (string), REDIS_ADDR (string), REDIS_PORT (integer),
// REDIS_PASSWORD (string), CHANNEL_PREFIX (string), DISPLAY_I2C_ADDR (UInt8)
#include "local_config.h"

#include <Redis.h>

// this sketch will build for the ESP8266 or ESP32 platform
#ifdef HAL_ESP32_HAL_H_ // ESP32
#include <WiFiClient.h>
#include <WiFi.h>
#else
#ifdef CORE_ESP8266_FEATURES_H // ESP8266
#include <ESP8266WiFi.h>
#endif
#endif

#include <functional>

#define MAX_BACKOFF 300000 // 5 minutes

LCM2004A_SDDPDisplayInterface displayInterface(DISPLAY_I2C_ADDR);
SDDPDisplay *display;

bool subscriberLoop(std::function<void(void)> resetBackoffCounter);

void setup()
{
    Serial.begin(115200);
    Serial.println();

    displayInterface.initialize();

    auto rawDisplay = displayInterface.rawDisplay(); 

    if (!display) {
      Serial.println("Display failed to initialize! Nothing doing, halting here.");
      return;
    }
    
    Serial.println("Display initialized");

    rawDisplay->setCursor(0, 0);
    rawDisplay->print("Connecting to SSID:");
    rawDisplay->setCursor(2, 1);
    rawDisplay->print(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int _i = 0;
    int _l = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(150);
        Serial.print(".");
        rawDisplay->setCursor(_i++ % 20, 2);
        if (_i % 20 == 0) {
          _l++;
        }
        rawDisplay->write(!(_l % 2) ? "." : " ");
    }

    Serial.println();
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    rawDisplay->clear();
    rawDisplay->setCursor(15, 0);
    rawDisplay->print("<" VERSION "." PROTOCOL_VERSION ">");
    rawDisplay->setCursor(0, 0);
    rawDisplay->print("Connected as:");
    rawDisplay->setCursor(2, 1);
    rawDisplay->print(HOSTNAME);
    rawDisplay->setCursor(0, 3);
    rawDisplay->print("IP: " + WiFi.localIP().toString());

    display = new SDDPDisplay(SDDPDisplay::Config{
      .displayName = String(HOSTNAME),
      .channelPrefix = String(CHANNEL_PREFIX),
      .redisHost = REDIS_ADDR,
      .redisPort = REDIS_PORT,
      .redisAuth = REDIS_PASSWORD,
      .clientCreator = []() -> Client* { return new WiFiClient(); }
    }, &displayInterface);

    auto backoffCounter = -1;
    auto resetBackoffCounter = [&]() { backoffCounter = 0; };

    resetBackoffCounter();
    while (subscriberLoop(resetBackoffCounter))
    {
        auto curDelay = min((1000 * (int)pow(2, backoffCounter)), MAX_BACKOFF);

        if (curDelay != MAX_BACKOFF)
        {
            ++backoffCounter;
        }

        Serial.printf("Waiting %lds to reconnect...\n", curDelay / 1000);
        delay(curDelay);
    }

    Serial.printf("Done!\n");
}

bool subscriberLoop(std::function<void(void)> resetBackoffCounter)
{
    auto subRv = display->startVending();

    return subRv == (SDDPDisplay::Returns)RedisSubscribeServerDisconnected || 
      subRv == (SDDPDisplay::Returns)RedisSubscribeOtherError;
}

void loop() {}