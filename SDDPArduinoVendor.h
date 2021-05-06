#ifndef SDDPDisplay_h
#define SDDPDisplay_h

#define PROTOCOL_VERSION "3"

class Client;
class Redis;

class SDDPDisplayInterface
{
public:
  // mgmt methods
  virtual void listening() {};
  virtual void consumerEstablished(String consumerId, String onChannel) = 0;
  virtual void consumerRejected(String consumerId, String onChannel, String reason) {};

  typedef enum {
    Left,
    Right,
    Up,
    Down
  } ScrollDir;

  // draw methods
  virtual void clear(void) = 0;
  virtual void home(void) = 0;
  virtual void writeAt(int column, int row, String toWrite) = 0;
  virtual void toggleDisplay(bool on) = 0;
  virtual void toggleCursor(bool on) = 0;
  virtual void toggleCursorBlink(bool on) = 0;
  virtual void scrollDisplay(ScrollDir direction) = 0;
};

class SDDPDisplay
{
public:
  typedef Client* (*CreateClientFunc)(void);

  typedef struct {
    String displayName;
    String channelPrefix;
    const char* redisHost;
    uint16_t redisPort;
    const char* redisAuth;
    CreateClientFunc clientCreator;
  } Config;

  typedef enum {
    // anything less than Success (0) is a RedisSubscribeResult
    Success = 0,
    NoDelegate,
    BadConfig,
    DisplayInitFailure
  } Returns;

  SDDPDisplay(Config config, SDDPDisplayInterface *delegate) : 
    config(config), delegate(delegate) {}

  ~SDDPDisplay() {}
  SDDPDisplay(const SDDPDisplay &) = delete;
  SDDPDisplay &operator=(const SDDPDisplay &) = delete;
  SDDPDisplay(const SDDPDisplay &&) = delete;
  SDDPDisplay &operator=(const SDDPDisplay &&) = delete;

  // blocks forever unless error
  Returns startVending(void);

private:
  typedef struct  {
    int start;
    int end;
    String value;
  } CommandToken;

  typedef struct  {
    Client* client;
    Redis* redis;
  } RedisConnection;

  Config config;
  SDDPDisplayInterface *delegate;
  String consumerId;
  String establishedChannel;
  RedisConnection publishConn;

  RedisConnection newConnection();
  void releaseConnection(RedisConnection conn);
  void messageCallback(Redis *, String, String);
};

#endif