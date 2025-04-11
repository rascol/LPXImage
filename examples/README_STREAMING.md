# LPXImage Streaming Demo

This directory contains Python scripts for streaming LPXImage video between two computers.

## Requirements

Before using these scripts, make sure the LPXImage Python module is properly installed on both computers.
See the main `INSTALL_PYTHON.md` file in the root directory for installation instructions.

## Available Scripts

1. **lpx_stream_demo.py** - Demonstrates the full streaming pipeline on a single computer
2. **lpx_server.py** - Server program to capture video, convert to LPXImage format, and stream
3. **lpx_renderer.py** - Client program to receive LPXImage frames and display them

## Setting Up Cross-Computer Streaming

### On the Server Computer:

1. Make sure the LPXImage Python module is installed
2. Run the server script with:

```bash
python lpx_server.py --camera 0 --width 640 --height 480
```

Options:
- `--camera`: Camera device ID (default: 0)
- `--width`: Video width (default: 640)
- `--height`: Video height (default: 480)
- `--tables`: Path to scan tables (default: ../ScanTables63)
- `--port`: Server port (default: 5050)

### On the Client Computer:

1. Make sure the LPXImage Python module is installed
2. Run the renderer script with:

```bash
python lpx_renderer.py --host SERVER_IP_ADDRESS
```

Replace `SERVER_IP_ADDRESS` with the IP address of the server computer.

Options:
- `--host`: Server hostname or IP address (default: 127.0.0.1)
- `--width`: Window width (default: 800)
- `--height`: Window height (default: 600)
- `--scale`: Rendering scale factor (default: 1.0)
- `--tables`: Path to scan tables (default: ../ScanTables63)

## Terminating the Programs

- On both computers, press Ctrl+C in the terminal to stop the program

## Troubleshooting

### ModuleNotFoundError: No module named 'lpximage'

This error means the LPXImage Python module isn't properly installed. Follow these steps:

1. Make sure you've built the LPXImage library with Python bindings enabled
2. Follow the installation instructions in INSTALL_PYTHON.md
3. Verify the installation by running:

```python
import lpximage
print(lpximage.__doc__)
```

### Connection Issues

- Make sure both computers are on the same network
- Check if there are firewalls blocking the connection on port 5050
- Try using the IP address instead of hostname
- Check if the server is running before starting the client

### Video Issues

- Try different camera IDs if your webcam isn't detected
- Adjust resolution to match your webcam's capabilities
- If video is slow, try reducing the resolution

## Example Usage Scenarios

### Basic Local Testing

```bash
# Terminal 1 (Server)
python lpx_server.py

# Terminal 2 (Client)
python lpx_renderer.py
```

### Cross-Computer High-Resolution Streaming

```bash
# Server Computer
python lpx_server.py --camera 0 --width 1280 --height 720

# Client Computer
python lpx_renderer.py --host 192.168.1.100 --width 1280 --height 720
```

### Low-Latency Configuration

```bash
# Server Computer
python lpx_server.py --width 320 --height 240

# Client Computer
python lpx_renderer.py --host 192.168.1.100 --scale 2.0
```
