/**
 * @file server.js
 * @brief Node.js TCP-to-WebSocket Bridge for Raw H.264 Video Stream
 */
const dgram = require('dgram');
const WebSocket = require('ws');

const UDP_PORT = 50002; // Cổng hứng dữ liệu từ C++ Gstreamer
const WS_PORT = 50004;  // Cổng phát WebSocket cho Trình duyệt Web

/* 1. KHỞI TẠO WEBSOCKET SERVER */
const wss = new WebSocket.Server({ port: WS_PORT }, () => {
    console.log(`[WebSocket] Server listening on ws://localhost:${WS_PORT}`);
});

// Hàm phát sóng (broadcast) dữ liệu cho tất cả các tab web đang mở
wss.broadcast = function (data) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
};

/* 2. KHỞI TẠO TCP SERVER HỨNG STREAM TỪ C++ */
const udpServer = dgram.createSocket('udp4');

udpServer.on('listening', () => {
    const address = udpServer.address();
    console.log(`[UDP] Bridge Active. Listening for H.264 packets on ${address.address}:${address.port}`);
});

// Mỗi khi bắt được 1 gói UDP từ Gstreamer, lập tức văng lên WebSocket
udpServer.on('message', (msg, rinfo) => {
    wss.broadcast(msg);
});

udpServer.on('error', (err) => {
    console.error(`[UDP Error] Server bỗng dưng gặp lỗi:\n${err.stack}`);
    udpServer.close();
});

// Gắn chặt vào cổng 5001 cục bộ
udpServer.bind(UDP_PORT, '127.0.0.1');