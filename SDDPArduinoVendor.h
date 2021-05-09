#ifndef SDDPDisplay_h
#define SDDPDisplay_h

#include <map>

#define PROTOCOL_VERSION "4"

#define DEFAULT_CONSUMER_TIMEOUT_SECONDS  15

class Client;
class Redis;

class SDDPDisplayInterface
{
public:
  // mgmt methods
  virtual void listening() {};
  virtual void consumerEstablished(String consumerId, String onChannel) = 0;
  virtual void consumerRejected(String consumerId, String onChannel, String reason) {};
  virtual void consumerDisconnected(String consumerId) {};

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

// TODO: need a "release" or "disconnect" *in the protocol*, and must unsubscribe to establishedChan
// and setup again for a new connection... it works *implicitly* right now, but that's not great!

class SDDPDisplay
{
public:
  typedef Client* (*CreateClientFunc)(void);

  typedef String (*CustomCommandHandlerFunc)(std::vector<String> arguments);

  typedef struct {
    String displayName;
    String channelPrefix;
    const char* redisHost;
    uint16_t redisPort;
    const char* redisAuth;
    CreateClientFunc clientCreator;
    uint32_t consumerTimeout; // set to 0 to default to DEFAULT_CONSUMER_TIMEOUT_SECONDS
  } Config;

  typedef enum {
    // anything less than Success (0) is a RedisSubscribeResult
    Success = 0,
    NoDelegate,
    BadConfig,
    DisplayInitFailure
  } Returns;

  SDDPDisplay(Config config, SDDPDisplayInterface *delegate) : 
    config(config), delegate(delegate), consumerId(""), establishedChannel(""), lastMsgMillis(-1) {}

  ~SDDPDisplay() {}
  SDDPDisplay(const SDDPDisplay &) = delete;
  SDDPDisplay &operator=(const SDDPDisplay &) = delete;
  SDDPDisplay(const SDDPDisplay &&) = delete;
  SDDPDisplay &operator=(const SDDPDisplay &&) = delete;

  bool registerCustomCommand(String command, CustomCommandHandlerFunc handler);

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

  typedef std::map<String, CustomCommandHandlerFunc> RegisteredCommandMap;

  Config config;
  SDDPDisplayInterface *delegate;
  String consumerId;
  String establishedChannel;
  unsigned long lastMsgMillis;
  
  RedisConnection publishConn;

  RegisteredCommandMap registeredCustomCommands;

  RedisConnection newConnection();
  void releaseConnection(RedisConnection conn);
  void messageCallback(Redis *, String, String);
  void establishedMessageCallback(Redis *, String, String, std::vector<CommandToken>&);
};

#endif