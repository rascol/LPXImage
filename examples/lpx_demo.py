# examples/lpx_demo.py
import numpy as np
import cv2
import lpximage  # Our Python module
import time
import threading
import signal  # For signal handling

def main():
    # Initialize scan tables
    tables = lpximage.LPXTables("../ScanTables63")
    if not tables.isInitialized():
        print("Failed to initialize scan tables")
        return
    
    # Just output the spiral period instead of calling printInfo
    print(f"Loaded scan tables with spiral period {tables.spiralPer}")
    
    # Test with a sample image
    img = cv2.imread("../lion.jpg")
    if img is None:
        # Try alternative paths
        img = cv2.imread("lion.jpg")
        if img is None:
            print("Failed to load sample image, please provide a valid path")
            return
    
    print(f"Loaded image with shape {img.shape}")
    
    # Convert BGR to RGB
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    # Initialize the LPX system first
    if not lpximage.initLPX("../ScanTables63", img.shape[1], img.shape[0]):
        print("Failed to initialize LPX system")
        return
    
    # Process the image using LPX
    start_time = time.time()
    
    # Center of the image
    center_x = img.shape[1] / 2
    center_y = img.shape[0] / 2
    
    # Scan the image
    lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)
    
    scan_time = time.time() - start_time
    print(f"Scanned image in {scan_time:.3f} seconds")
    print(f"LPX image contains {lpx_image.getLength()} cells")
    
    # Initialize renderer
    renderer = lpximage.LPXRenderer()
    renderer.setScanTables(tables)
    
    # Render the image
    start_time = time.time()
    rendered = renderer.renderToImage(lpx_image, 800, 600, 1.0)
    render_time = time.time() - start_time
    print(f"Rendered image in {render_time:.3f} seconds")
    
    # Convert the numpy array back to OpenCV format for display
    render_bgr = cv2.cvtColor(rendered, cv2.COLOR_RGB2BGR)
    
    # Add signal handler for Ctrl+C
    import signal
    
    # Define signal handler function
    def signal_handler(sig, frame):
        print("\nCtrl+C pressed, exiting...")
        cv2.destroyAllWindows()
        exit(0)
    
    # Register the signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Display the original and rendered images
    cv2.imshow("Original", img)
    cv2.imshow("LPX Rendered", render_bgr)
    
    print("Press any key in the image window to continue or Ctrl+C in the terminal to exit")
    
    # Wait for a key press with timeout to allow for Ctrl+C handling
    while True:
        key = cv2.waitKey(100)  # Short timeout allows for Ctrl+C handling
        if key != -1:
            break
        
    cv2.destroyAllWindows()
    
    # Example of using the webcam server
    def run_webcam_server():
        server = lpximage.WebcamLPXServer("../ScanTables63")
        server.start(0, 1920, 1080)
        print("Webcam server started")
        
        # Run for 60 seconds
        time.sleep(60)
        
        server.stop()
        print("Webcam server stopped")
    
    # Example of using the debug client
    def run_debug_client():
        client = lpximage.LPXDebugClient("../ScanTables63")
        client.setWindowTitle("Python LPX Debug View")
        client.setWindowSize(800, 600)
        client.setScale(1.0)
        
        # Initialize window on main thread
        client.initializeWindow()
        
        # Connect to server
        if not client.connect("localhost"):
            print("Failed to connect to server")
            return
        
        print("Connected to LPX server")
        
        # Process events on main thread
        while client.isRunning():
            if not client.processEvents():
                break
            time.sleep(0.01)
        
        client.disconnect()
        print("Disconnected from server")
    
    # Example of how to use server and client
    print("Example of how to run server and client (commented out in this demo)")
    # Uncomment to test:
    # server_thread = threading.Thread(target=run_webcam_server)
    # server_thread.start()
    # time.sleep(2)  # Give server time to start
    # run_debug_client()
    # server_thread.join()

if __name__ == "__main__":
    main()
