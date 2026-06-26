#pragma once
#include "../WindowsAPI.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
typedef int SOCKET;
#define INVALID_SOCKET  ((SOCKET)(-1))
#define SOCKET_ERROR    (-1)
#define WSAEINPROGRESS  EINPROGRESS
#define WSAEWOULDBLOCK  EWOULDBLOCK
#define WSAETIMEDOUT    ETIMEDOUT
#define WSAEINVAL       EINVAL
#define WSAECONNRESET   ECONNRESET
#define WSAECONNREFUSED ECONNREFUSED
#define WSAENOTINITIALISED 0
