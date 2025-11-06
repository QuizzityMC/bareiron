#ifndef H_WASM_COMPAT
#define H_WASM_COMPAT

#ifdef WASM_BUILD

#include <emscripten.h>
#include <emscripten/websocket.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

// Socket compatibility layer for WebAssembly
// Maps BSD socket API to Emscripten WebSocket API

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define O_NONBLOCK 0x0004
#define MSG_PEEK 0x02
#define SHUT_WR 1

#define EAGAIN 11
#define EWOULDBLOCK EAGAIN

// WebSocket-based socket structure
typedef struct {
  EMSCRIPTEN_WEBSOCKET_T ws;
  int connected;
  uint8_t recv_buffer[4096];
  int recv_buffer_len;
  int recv_buffer_pos;
} wasm_socket_t;

// Global socket pool
extern wasm_socket_t wasm_sockets[32];
extern int wasm_socket_count;

// Socket API compatibility functions
int wasm_socket(int domain, int type, int protocol);
int wasm_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int wasm_listen(int sockfd, int backlog);
int wasm_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int wasm_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t wasm_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t wasm_recv(int sockfd, void *buf, size_t len, int flags);
int wasm_close(int sockfd);
int wasm_fcntl(int fd, int cmd, ...);
int wasm_shutdown(int sockfd, int how);

// Map standard socket functions to WASM implementations
#define socket wasm_socket
#define bind wasm_bind
#define listen wasm_listen
#define accept wasm_accept
#define connect wasm_connect
#define send wasm_send
#define recv wasm_recv
#define close wasm_close
#define fcntl wasm_fcntl
#define shutdown wasm_shutdown

// Stub sockaddr structures (not fully used in WASM)
struct sockaddr {
  unsigned short sa_family;
  char sa_data[14];
};

struct sockaddr_in {
  short sin_family;
  unsigned short sin_port;
  struct in_addr {
    unsigned long s_addr;
  } sin_addr;
  char sin_zero[8];
};

typedef unsigned int socklen_t;

#define AF_INET 2
#define SOCK_STREAM 1
#define INADDR_ANY 0

// htons/ntohs macros
#define htons(x) ((uint16_t)((((x) & 0xff) << 8) | (((x) >> 8) & 0xff)))
#define ntohs(x) htons(x)

// Initialize WebSocket server (to be called from JavaScript)
void wasm_init_websocket_server(int port);

// WebSocket callbacks
EM_BOOL wasm_websocket_open_callback(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData);
EM_BOOL wasm_websocket_message_callback(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData);
EM_BOOL wasm_websocket_close_callback(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData);
EM_BOOL wasm_websocket_error_callback(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData);

#endif // WASM_BUILD

#endif // H_WASM_COMPAT
