#!/usr/bin/env python3
"""
Create a test video with colorful frames for testing color channel consistency
"""
import cv2
import numpy as np

def create_test_video(filename="test_color_video.mp4"):
    """Create a test video with different colored frames"""
    width, height = 640, 480
    fps = 30
    
    # Create video writer
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(filename, fourcc, fps, (width, height))
    
    # Create frames with different solid colors
    colors = [
        (0, 0, 255),    # Red (BGR)
        (0, 255, 0),    # Green (BGR)
        (255, 0, 0),    # Blue (BGR)
        (0, 255, 255),  # Yellow (BGR)
        (255, 0, 255),  # Magenta (BGR)
        (255, 255, 0),  # Cyan (BGR)
    ]
    
    # Write multiple frames for each color (30 frames = 1 second at 30fps)
    for color in colors:
        frame = np.full((height, width, 3), color, dtype=np.uint8)
        
        # Add some text to identify the color
        color_names = {
            (0, 0, 255): "RED",
            (0, 255, 0): "GREEN", 
            (255, 0, 0): "BLUE",
            (0, 255, 255): "YELLOW",
            (255, 0, 255): "MAGENTA",
            (255, 255, 0): "CYAN"
        }
        
        text = color_names.get(color, "UNKNOWN")
        cv2.putText(frame, text, (width//2-50, height//2), cv2.FONT_HERSHEY_SIMPLEX, 
                    1, (255, 255, 255), 2, cv2.LINE_AA)
        
        # Write 30 frames (1 second) for each color
        for _ in range(30):
            out.write(frame)
    
    out.release()
    print(f"Created test video: {filename}")
    return filename

if __name__ == "__main__":
    create_test_video("/Users/ray/Desktop/LPXImage/test_colors.mp4")
