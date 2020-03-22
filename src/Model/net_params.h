// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// Limits
#define  MAX_NAME_LENGTH                     20
#define  SERVER_PORT                         51337
#define  MAX_BUFFER_SIZE                     1410
#define  MAX_TCP_BUFFER_SIZE                 9000
#define  MAX_VERSION_STRING_LENGTH           20


// TCP / UDP
#define  INTERVAL_TCP_ACCEPT_MS              300
#define  INTERVAL_TCP_MESSAGE_MS             150   // must be a multiple of 25, see listenForMessage().
#define  INTERVAL_UDP_MESSAGE_MS             2
#define  ANTI_SPAM_MINIMUM_TIME_SEC          2.0f
#define  WRONG_PASSWORD_INTERVAL_SEC         5.0f
#define  INTERVAL_REFRESH_WRONG_PACKETS_SEC  30
#define  NOTIFY_WHEN_WRONG_PACKETS_MORE      50   // in INTERVAL_REFRESH_WRONG_PACKETS_SEC


// Ping / Keepalive
#define  PING_CHECK_INTERVAL_SEC             50   // also change in the client
#define  PING_ANSWER_WAIT_TIME_SEC           10
#define  INTERVAL_KEEPALIVE_SEC              20   // also change in the client
#define  MAX_TIMEOUT_TIME_MS                 10000
