#include <vector>
#include <Redis.h>
#include "SDDPArduinoVendor.h"

SDDPDisplay::RedisConnection SDDPDisplay::newConnection()
{
  RedisConnection retVal = { .client = nullptr, .redis = nullptr };
  Client* conn = config.clientCreator();

  if (!conn->connect(config.redisHost, config.redisPort))
  {
      Serial.println("Failed to connect to the Redis server!");
      return retVal;
  }

  Redis* redis = new Redis(*conn);
  if (config.redisAuth && redis->authenticate(config.redisAuth) == RedisSuccess)
  {
      retVal = { .client = conn, .redis = redis };
  }
  else
  {
      Serial.printf("Failed to authenticate to the Redis server!\n");
  }

  return retVal;
}

void SDDPDisplay::releaseConnection(SDDPDisplay::RedisConnection conn)
{
  delete conn.redis;
  delete conn.client;
}

void SDDPDisplay::messageCallback(Redis *redisInst, String channel, String msg)
{
  std::vector<CommandToken> comps;
  int tokBegin = 0;

  for (int i = 0; i <= msg.length(); i++) {
    if (msg.charAt(i) == ' ' || i == msg.length()) {
      comps.push_back(CommandToken{
        .start = tokBegin, 
        .end = i, 
        .value = msg.substring(tokBegin, i)
      });

      tokBegin = i + 1;
    }
  }
  
  if (channel == (config.channelPrefix + "ctrl-init")) {
    if (comps.size() < 2) {
      return;
    }

    auto fromId = comps[0].value;
    auto command = comps[1].value;
    
    if (comps.size() > 2) {
      // <fromId> request <displayName> <protocolVersion>
      if (command == "request" && comps[2].value == config.displayName) {
        // exclusive?
        auto publishConn = newConnection();

        if (publishConn.redis) {
          auto goodProtoVer = comps.size() == 4 && String(PROTOCOL_VERSION) == comps[3].value;

          if (!goodProtoVer) {
            publishConn.redis->publish((config.channelPrefix + "ctrl-init:request:resp").c_str(),
              (config.displayName + " " + "reject " + fromId + " bad_protocol_version").c_str());
            delegate->consumerRejected(fromId, establishedChannel, "bad_protocol_version");
          }
          else {
            if (redisInst->subscribe(establishedChannel.c_str())) {
              publishConn.redis->publish((config.channelPrefix + "ctrl-init:request:resp").c_str(),
                (config.displayName + " " + "ok " + fromId).c_str());

              establishedChannel = config.channelPrefix + "estab:" + config.displayName + "|" + fromId;
              consumerId = fromId;

              delegate->consumerEstablished(consumerId, establishedChannel);
            } else {
              /// hmm what to do here!?
              Serial.println("FAILED TO SUB TO ESTAB CHAN");
            }
          }
          
          releaseConnection(publishConn);
        }
      }
    }
  }
  else if (establishedChannel.length() > 0 && consumerId.length() > 0) {
    if (channel == establishedChannel) {
      auto seqNo = comps[0].value.toInt();
      auto command = comps[1].value;
      
      if (command == "clear") {
        delegate->clear();
      }
      else if (command == "home") {
        delegate->home();
      }
      else if (command == "writeat" && comps.size() >= 4) { // not 5 because ' ' is a valid string to write
        auto col = comps[2].value.toInt();
        auto row = comps[3].value.toInt();

        auto restOf = msg.substring(comps[3].end + 1, msg.length());

        delegate->writeAt(col, row, restOf);
      }
      else if (command == "scroll" && comps.size() >= 3) {
        if (comps[2].value == "left") {
          delegate->scrollDisplay(SDDPDisplayInterface::ScrollDir::Left);
        }
        else if (comps[2].value == "right") {
          delegate->scrollDisplay(SDDPDisplayInterface::ScrollDir::Right);
        }
      }
      else if (command == "display" && comps.size() >= 3) {
        if (comps[2].value == "on" || comps[2].value == "1") {
          delegate->toggleDisplay(true);
        }
        else if (comps[2].value == "off" || comps[2].value == "0") {
          delegate->toggleDisplay(false);
        }
      }
      else if (command == "cursor" && comps.size() >= 3) {
        if (comps[2].value == "on" || comps[2].value == "1") {
          delegate->toggleCursor(true);
        }
        else if (comps[2].value == "off" || comps[2].value == "0") {
          delegate->toggleCursor(false);
        }
      }
      else if (command == "blink" && comps.size() >= 3) {
        if (comps[2].value == "on" || comps[2].value == "1") {
          delegate->toggleCursorBlink(true);
        }
        else if (comps[2].value == "off" || comps[2].value == "0") {
          delegate->toggleCursorBlink(false);
        }
      }
      
      if (publishConn.redis) {
        publishConn.redis->publish(String(channel + ":ack").c_str(), String(seqNo).c_str());
      }
    }
  }
}

SDDPDisplay::Returns SDDPDisplay::startVending(void)
{
  if (!delegate) {
    return NoDelegate;
  }
  
  RedisConnection mainConn = newConnection();
  publishConn = newConnection();

  if (!mainConn.client || !mainConn.redis || !publishConn.client || !publishConn.redis) {
    return BadConfig;
  }

  mainConn.redis->psubscribe((config.channelPrefix + String("ctrl-*")).c_str());
  mainConn.redis->setTestContext((void*)this);

  delegate->listening();
  
  auto subRv = mainConn.redis->startSubscribing(
    [=](Redis *redisInst, String channel, String msg) {
      ((SDDPDisplay*)redisInst->getTestContext())->messageCallback(redisInst, channel, msg);
    },
    [=](Redis *redisInst, RedisMessageError err) {
      Serial.printf("Subscription error! '%d'\n", err);
    }
  );

  mainConn.client->stop();
  releaseConnection(mainConn);

  return (Returns)subRv;
}