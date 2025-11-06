#ifdef WASM_BUILD

#include "wasm_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global socket pool
wasm_socket_t wasm_sockets[32];
int wasm_socket_count = 0;

// Server socket (listens for connections via JavaScript WebSocket server)
static int server_socket_fd = -1;
static int next_client_fd = 1;

int wasm_socket(int domain, int type, int protocol) {
  // Find free socket slot
  for (int i = 0; i < 32; i++) {
    if (wasm_sockets[i].ws == 0 && wasm_sockets[i].connected == 0) {
      wasm_sockets[i].connected = 0;
      wasm_sockets[i].recv_buffer_len = 0;
      wasm_sockets[i].recv_buffer_pos = 0;
      return i;
    }
  }
  return -1;
}

int wasm_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  // In WASM, binding is handled by JavaScript WebSocket server
  if (sockfd >= 0 && sockfd < 32) {
    server_socket_fd = sockfd;
    return 0;
  }
  return -1;
}

int wasm_listen(int sockfd, int backlog) {
  // Listening is handled by JavaScript WebSocket server
  return 0;
}

int wasm_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  // Accept is non-blocking and returns -1 when no connections are pending
  // Actual connections are handled by JavaScript and added via wasm_add_client
  // This is just a stub that always returns "would block"
  errno = EAGAIN;
  return -1;
}

int wasm_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  // Not used by server
  return -1;
}

ssize_t wasm_send(int sockfd, const void *buf, size_t len, int flags) {
  if (sockfd < 0 || sockfd >= 32 || !wasm_sockets[sockfd].connected) {
    return -1;
  }
  
  // Send data through WebSocket
  EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(
    wasm_sockets[sockfd].ws, (void*)buf, len
  );
  
  if (result < 0) {
    return -1;
  }
  
  return len;
}

ssize_t wasm_recv(int sockfd, void *buf, size_t len, int flags) {
  if (sockfd < 0 || sockfd >= 32 || !wasm_sockets[sockfd].connected) {
    return -1;
  }
  
  wasm_socket_t *sock = &wasm_sockets[sockfd];
  
  // Check if we have data in the buffer
  int available = sock->recv_buffer_len - sock->recv_buffer_pos;
  if (available <= 0) {
    // No data available, would block
    errno = EAGAIN;
    return -1;
  }
  
  // Determine how much to read
  size_t to_read = (len < available) ? len : available;
  
  if (flags & MSG_PEEK) {
    // Peek mode: copy data without consuming it
    memcpy(buf, sock->recv_buffer + sock->recv_buffer_pos, to_read);
  } else {
    // Normal mode: copy and consume data
    memcpy(buf, sock->recv_buffer + sock->recv_buffer_pos, to_read);
    sock->recv_buffer_pos += to_read;
    
    // Reset buffer if we've consumed everything
    if (sock->recv_buffer_pos >= sock->recv_buffer_len) {
      sock->recv_buffer_len = 0;
      sock->recv_buffer_pos = 0;
    }
  }
  
  return to_read;
}

int wasm_close(int sockfd) {
  if (sockfd < 0 || sockfd >= 32) {
    return -1;
  }
  
  if (wasm_sockets[sockfd].ws != 0) {
    emscripten_websocket_close(wasm_sockets[sockfd].ws, 0, "");
    emscripten_websocket_delete(wasm_sockets[sockfd].ws);
  }
  
  wasm_sockets[sockfd].ws = 0;
  wasm_sockets[sockfd].connected = 0;
  wasm_sockets[sockfd].recv_buffer_len = 0;
  wasm_sockets[sockfd].recv_buffer_pos = 0;
  
  return 0;
}

int wasm_fcntl(int fd, int cmd, ...) {
  // Non-blocking mode is default in WASM
  return 0;
}

int wasm_shutdown(int sockfd, int how) {
  // Shutdown is similar to close for WebSockets
  if (sockfd >= 0 && sockfd < 32 && wasm_sockets[sockfd].ws != 0) {
    emscripten_websocket_close(wasm_sockets[sockfd].ws, 0, "");
  }
  return 0;
}

// WebSocket callbacks
EM_BOOL wasm_websocket_open_callback(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData) {
  int sockfd = (int)(intptr_t)userData;
  if (sockfd >= 0 && sockfd < 32) {
    wasm_sockets[sockfd].connected = 1;
    printf("WebSocket client connected on fd %d\n", sockfd);
  }
  return EM_TRUE;
}

EM_BOOL wasm_websocket_message_callback(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData) {
  int sockfd = (int)(intptr_t)userData;
  if (sockfd >= 0 && sockfd < 32) {
    wasm_socket_t *sock = &wasm_sockets[sockfd];
    
    // Append data to receive buffer
    size_t space_available = sizeof(sock->recv_buffer) - sock->recv_buffer_len;
    size_t to_copy = (e->numBytes < space_available) ? e->numBytes : space_available;
    
    if (to_copy > 0) {
      memcpy(sock->recv_buffer + sock->recv_buffer_len, e->data, to_copy);
      sock->recv_buffer_len += to_copy;
    }
  }
  return EM_TRUE;
}

EM_BOOL wasm_websocket_close_callback(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData) {
  int sockfd = (int)(intptr_t)userData;
  if (sockfd >= 0 && sockfd < 32) {
    printf("WebSocket client disconnected on fd %d\n", sockfd);
    wasm_sockets[sockfd].connected = 0;
  }
  return EM_TRUE;
}

EM_BOOL wasm_websocket_error_callback(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData) {
  int sockfd = (int)(intptr_t)userData;
  printf("WebSocket error on fd %d\n", sockfd);
  return EM_TRUE;
}

// Called from JavaScript when a new WebSocket connection is established
EMSCRIPTEN_KEEPALIVE
int wasm_add_client_connection(EMSCRIPTEN_WEBSOCKET_T ws) {
  // Find free socket slot
  int sockfd = wasm_socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    return -1;
  }
  
  wasm_sockets[sockfd].ws = ws;
  wasm_sockets[sockfd].connected = 1;
  wasm_sockets[sockfd].recv_buffer_len = 0;
  wasm_sockets[sockfd].recv_buffer_pos = 0;
  
  // Set up callbacks
  emscripten_websocket_set_onopen_callback(ws, (void*)(intptr_t)sockfd, wasm_websocket_open_callback);
  emscripten_websocket_set_onmessage_callback(ws, (void*)(intptr_t)sockfd, wasm_websocket_message_callback);
  emscripten_websocket_set_onclose_callback(ws, (void*)(intptr_t)sockfd, wasm_websocket_close_callback);
  emscripten_websocket_set_onerror_callback(ws, (void*)(intptr_t)sockfd, wasm_websocket_error_callback);
  
  printf("Added WebSocket client connection, fd: %d\n", sockfd);
  
  return sockfd;
}

#endif // WASM_BUILD
