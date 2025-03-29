/**
 * Web server for streaming log-polar images
 */

const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const fs = require('fs');
const lpx = require('./index');

// Configuration
const PORT = process.env.PORT || 3000;
const SCAN_TABLES_FILE = path.join(__dirname, 'ScanTables63');
const FRAME_RATE = 30; // Frames per second

// Initialize Express app
const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// Create public directory if it doesn't exist
const publicDir = path.join(__dirname, 'public');
if (!fs.existsSync(publicDir)) {
  fs.mkdirSync(publicDir);
}

// Create a simple HTML page for viewing the stream
const htmlContent = `
<!DOCTYPE html>
<html>
<head>
  <title>Log-Polar Vision Stream</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { display: flex; flex-wrap: wrap; }
    .video-container { margin: 10px; }
    canvas { border: 1px solid #ccc; }
    h2 { margin-top: 10px; }
  </style>
</head>
<body>
  <h1>Log-Polar Vision Stream</h1>
  <div class="container">
    <div class="video-container">
      <h2>Original Video</h2>
      <canvas id="originalCanvas" width="640" height="480"></canvas>
    </div>
    <div class="video-container">
      <h2>Log-Polar Representation</h2>
      <canvas id="logPolarCanvas" width="640" height="480"></canvas>
    </div>
  </div>

  <script>
    // Connect to WebSocket server
    const ws = new WebSocket('ws://' + window.location.host);
    const originalCanvas = document.getElementById('originalCanvas');
    const logPolarCanvas = document.getElementById('logPolarCanvas');
    const originalCtx = originalCanvas.getContext('2d');
    const logPolarCtx = logPolarCanvas.getContext('2d');

    ws.onmessage = function(event) {
      // Parse the JSON message
      const data = JSON.parse(event.data);
      
      // Draw the original image
      if (data.original) {
        const originalImg = new Image();
        originalImg.onload = function() {
          originalCtx.drawImage(originalImg, 0, 0, originalCanvas.width, originalCanvas.height);
        };
        originalImg.src = 'data:image/jpeg;base64,' + data.original;
      }
      
      // Draw the log-polar image
      if (data.logPolar) {
        const logPolarImg = new Image();
        logPolarImg.onload = function() {
          logPolarCtx.drawImage(logPolarImg, 0, 0, logPolarCanvas.width, logPolarCanvas.height);
        };
        logPolarImg.src = 'data:image/jpeg;base64,' + data.logPolar;
      }
    };
  </script>
</body>
</html>
`;

fs.writeFileSync(path.join(publicDir, 'index.html'), htmlContent);

// WebSocket connections
wss.on('connection', (ws) => {
  console.log('Client connected');
  
  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

// Initialize LPX system
let lpxInitialized = false;
try {
  if (fs.existsSync(SCAN_TABLES_FILE)) {
    lpxInitialized = lpx.initialize(SCAN_TABLES_FILE);
    console.log(`LPX system initialized: ${lpxInitialized}`);
  } else {
    console.error(`Scan tables file not found: ${SCAN_TABLES_FILE}`);
  }
} catch (err) {
  console.error('Error initializing LPX system:', err);
}

// Start camera if LPX system initialized
let cameraStarted = false;
if (lpxInitialized) {
  try {
    cameraStarted = lpx.startCamera(0);
    console.log(`Camera started: ${cameraStarted}`);
  } catch (err) {
    console.error('Error starting camera:', err);
  }
}

// Frame processing function
function processFrame() {
  if (!lpxInitialized || !cameraStarted) {
    return;
  }
  
  try {
    // Capture frame and convert to log-polar
    const lpxData = lpx.captureFrame(0.5, 0.5);
    
    // Render both original and log-polar images
    const logPolarImage = lpx.renderLogPolarImage(lpxData);
    
    // Broadcast to all connected clients
    const clients = wss.clients;
    if (clients.size > 0) {
      const message = JSON.stringify({
        timestamp: Date.now(),
        logPolar: logPolarImage.toString('base64')
      });
      
      clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(message);
        }
      });
    }
  } catch (err) {
    console.error('Error processing frame:', err);
  }
}

// Start the frame processing loop if camera started
if (cameraStarted) {
  setInterval(processFrame, 1000 / FRAME_RATE);
}

// Start the server
server.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}/`);
});

// Handle shutdown
process.on('SIGINT', () => {
  console.log('Shutting down...');
  
  if (cameraStarted) {
    lpx.stopCamera();
  }
  
  if (lpxInitialized) {
    lpx.shutdown();
  }
  
  process.exit(0);
});
