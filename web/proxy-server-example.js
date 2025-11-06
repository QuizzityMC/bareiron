#!/usr/bin/env node

/**
 * WebSocket-to-TCP Proxy Server
 * 
 * This proxy bridges between Minecraft clients (TCP) and the browser-based
 * bareiron server (WebSocket). Run this alongside the web server.
 * 
 * Usage:
 *   node proxy-server-example.js [options]
 * 
 * Options:
 *   --tcp-port <port>    Port for Minecraft clients (default: 25565)
 *   --ws-url <url>       WebSocket URL to browser server (default: ws://localhost:8080)
 *   --help               Show this help
 */

const net = require('net');
const WebSocket = require('ws');

// Parse command line arguments
const args = process.argv.slice(2);
let tcpPort = 25565;
let wsUrl = 'ws://localhost:8080';

for (let i = 0; i < args.length; i++) {
  switch (args[i]) {
    case '--tcp-port':
      tcpPort = parseInt(args[++i]);
      break;
    case '--ws-url':
      wsUrl = args[++i];
      break;
    case '--help':
      console.log('WebSocket-to-TCP Proxy Server');
      console.log('Usage: node proxy-server-example.js [options]');
      console.log('Options:');
      console.log('  --tcp-port <port>    Port for Minecraft clients (default: 25565)');
      console.log('  --ws-url <url>       WebSocket URL (default: ws://localhost:8080)');
      console.log('  --help               Show this help');
      process.exit(0);
    default:
      console.error(`Unknown option: ${args[i]}`);
      process.exit(1);
  }
}

console.log('WebSocket-to-TCP Proxy Server');
console.log('==============================');
console.log(`TCP Port: ${tcpPort} (for Minecraft clients)`);
console.log(`WebSocket URL: ${wsUrl}`);
console.log('');

// Track active connections
const connections = new Map();
let nextConnectionId = 1;

// Create TCP server for Minecraft clients
const tcpServer = net.createServer((tcpSocket) => {
  const connId = nextConnectionId++;
  console.log(`[${connId}] New Minecraft client connected from ${tcpSocket.remoteAddress}`);
  
  // Connect to WebSocket server
  const ws = new WebSocket(wsUrl);
  
  // Store connection info
  connections.set(connId, {
    tcp: tcpSocket,
    ws: ws,
    connected: false
  });
  
  // WebSocket opened
  ws.on('open', () => {
    console.log(`[${connId}] WebSocket connected to browser`);
    connections.get(connId).connected = true;
  });
  
  // WebSocket received data from browser server
  ws.on('message', (data) => {
    if (tcpSocket.writable) {
      tcpSocket.write(data);
    }
  });
  
  // WebSocket closed
  ws.on('close', () => {
    console.log(`[${connId}] WebSocket closed`);
    if (!tcpSocket.destroyed) {
      tcpSocket.end();
    }
    connections.delete(connId);
  });
  
  // WebSocket error
  ws.on('error', (error) => {
    console.error(`[${connId}] WebSocket error:`, error.message);
    if (!tcpSocket.destroyed) {
      tcpSocket.end();
    }
    connections.delete(connId);
  });
  
  // TCP received data from Minecraft client
  tcpSocket.on('data', (data) => {
    const conn = connections.get(connId);
    if (conn && conn.connected && ws.readyState === WebSocket.OPEN) {
      ws.send(data);
    }
  });
  
  // TCP socket closed
  tcpSocket.on('close', () => {
    console.log(`[${connId}] Minecraft client disconnected`);
    if (ws.readyState === WebSocket.OPEN) {
      ws.close();
    }
    connections.delete(connId);
  });
  
  // TCP socket error
  tcpSocket.on('error', (error) => {
    console.error(`[${connId}] TCP error:`, error.message);
    if (ws.readyState === WebSocket.OPEN) {
      ws.close();
    }
    connections.delete(connId);
  });
});

// Start TCP server
tcpServer.listen(tcpPort, () => {
  console.log(`Proxy server listening on port ${tcpPort}`);
  console.log('Minecraft clients can now connect to localhost:' + tcpPort);
  console.log('');
  console.log('Note: Make sure the browser server is running at ' + wsUrl);
  console.log('Press Ctrl+C to stop');
});

// Handle server errors
tcpServer.on('error', (error) => {
  if (error.code === 'EADDRINUSE') {
    console.error(`Error: Port ${tcpPort} is already in use!`);
    console.error('Try a different port with --tcp-port option');
  } else {
    console.error('Server error:', error.message);
  }
  process.exit(1);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down proxy server...');
  
  // Close all connections
  for (const [connId, conn] of connections) {
    console.log(`Closing connection ${connId}`);
    if (conn.ws.readyState === WebSocket.OPEN) {
      conn.ws.close();
    }
    if (!conn.tcp.destroyed) {
      conn.tcp.end();
    }
  }
  
  // Close server
  tcpServer.close(() => {
    console.log('Proxy server stopped');
    process.exit(0);
  });
});
