#pragma once


// Limits
#define  MAX_NAME_LENGTH                     20
#define  SERVER_PORT                         51337
#define  MAX_BUFFER_SIZE                     1410
#define  MAX_VERSION_STRING_LENGTH           20


// TCP / UDP
#define  INTERVAL_TCP_ACCEPT_MS              300
#define  INTERVAL_TCP_MESSAGE_MS             150 // must be a multiple of 25, see listenForMessage().
#define  INTERVAL_UDP_MESSAGE_MS             2
#define  ANTI_SPAM_MINIMUM_TIME_SEC          2.0f
#define  INTERVAL_REFRESH_WRONG_PACKETS_SEC  30
#define  NOTIFY_WHEN_WRONG_PACKETS_MORE      50 // in INTERVAL_REFRESH_WRONG_PACKETS_SEC


// Ping / Keepalive
#define  PING_CHECK_INTERVAL_SEC             3 // also change in the client
// when changing change client's INTERVAL_KEEPALIVE_SEC macro too
#define  INTERVAL_KEEPALIVE_SEC              20
#define  MAX_TIMEOUT_TIME_MS                 10000


// Ping color
#define  PING_NORMAL_BELOW                   300
#define  PING_WARNING_BELOW                  500
