import os

def file_operations():
    # File name
    filename = "student_info.txt"
    
    try:
        # Create and open file for writing (system call: open)
        # os.O_CREAT: create file if it doesn't exist
        # os.O_WRONLY: open for writing only
        # os.O_TRUNC: truncate file to 0 bytes if it exists
        file_descriptor = os.open(filename, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644)
        
        # Text to write
        student_info = "Student Number: 00611723\nName: Andrew McCord\nCourse: Operating Systems\n"
        
        # Write to file (system call: write)
        # Convert string to bytes and write to file descriptor
        os.write(file_descriptor, student_info.encode('utf-8'))
        
        # Close the file (system call: close)
        os.close(file_descriptor)
        
        print("File created and data written successfully.")
        
        # Reopen file for reading (system call: open)
        # os.O_RDONLY: open for reading only
        file_descriptor = os.open(filename, os.O_RDONLY)
        
        # Read file contents (system call: read)
        # Read up to 1024 bytes
        content = os.read(file_descriptor, 1024)
        
        # Close the file (system call: close)
        os.close(file_descriptor)
        
        # Print contents to terminal
        print("\nFile contents:")
        print(content.decode('utf-8'))
        
        # Delete the file (system call: unlink/remove)
        os.remove(filename)
        print(f"File '{filename}' deleted successfully.")
        
    except OSError as e:
        print(f"Error occurred: {e}")

if __name__ == "__main__":
    file_operations()