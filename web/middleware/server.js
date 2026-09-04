const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
app.use(cors());

const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  }
});

// Spawn the C++ Nexus-HFT Engine
// const enginePath = path.resolve(__dirname, '../../build/nexus_engine_v2.exe');
const engineName =
    process.platform === 'win32'
        ? 'nexus_engine_v2.exe'
        : 'nexus_engine_v2';

const enginePath = path.resolve(
    __dirname,
    '../../build',
    engineName
);
console.log(`Starting Nexus-HFT Engine from: ${enginePath}`);
const engine = spawn(enginePath);

let pendingTrades = [];

engine.stdout.on('data', (data) => {
  const lines = data.toString().trim().split('\n');
  lines.forEach(line => {
    const parts = line.trim().split(' ');
    if (parts[0] === 'BOOK') {
      try {
        const payload = JSON.parse(line.substring(5)); // skip "BOOK "
        // prices are already decimal from the engine
        // payload.bids = payload.bids.map(b => [b[0] / 100, b[1]]);
        // payload.asks = payload.asks.map(a => [a[0] / 100, a[1]]);
        io.emit('book_update', payload);
      } catch (e) {
        console.error("Failed to parse BOOK JSON:", e);
      }
    } else if (parts[0] === 'TRADE') {
      pendingTrades.push({
        price: parseFloat(parts[1]),
        qty: parseInt(parts[2], 10)
      });
    } else if (parts[0] === 'LATENCY') {
      const latency = parseInt(parts[1], 10);
      io.emit('latency', { nanoseconds: latency });
      
      // Emit all pending trades with this latency
      pendingTrades.forEach(t => {
        t.latency = latency;
        io.emit('trade', t);
      });
      pendingTrades = [];
    } else if (parts[0] === 'CLEAR_ACK') {
      io.emit('cleared');
    } else {
        console.log(`[ENGINE] ${line}`);
    }
  });
});

engine.stderr.on('data', (data) => {
  console.error(`[ENGINE ERR] ${data.toString()}`);
});

engine.on('close', (code) => {
  console.log(`Engine process exited with code ${code}`);
});

io.on('connection', (socket) => {
  console.log('A client connected:', socket.id);

  socket.on('submit_order', (order) => {
    // order format: { side: 'BUY' or 'SELL', price: 100, qty: 10 }
    if (order.side && order.price && order.qty) {
        const cmd = `${order.side} ${order.price} ${order.qty}\n`;
        console.log(`Routing to engine: ${cmd.trim()}`);
        engine.stdin.write(cmd);
    }
  });

  socket.on('clear_book', () => {
    console.log(`Routing to engine: CLEAR`);
    engine.stdin.write("CLEAR\n");
  });

  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
  });
});

const PORT = process.env.PORT || 3001;

server.listen(PORT, '0.0.0.0', () => {
  console.log(`Middleware bridge running on port ${PORT}`);
});
