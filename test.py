import os
import sys
import subprocess
import http.server
import socketserver
import threading
import time
import filecmp

# Configuration
PORT = 8080
TEST_FILE = "LICENSE"
TEST_URL = f"http://localhost:{PORT}/{TEST_FILE}"
WURL_EXECUTABLE = os.path.abspath("./wurl")  # Use the absolute path to wurl
WURL_OUTPUT = "/tmp/wurl.txt"
WGET_OUTPUT = "/tmp/wget.txt"
WEB_DIR = "/tmp/webserver"

# Create a simple HTTP server to serve the test file
class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        # Suppress server logging
        pass

def start_http_server():
    os.makedirs(WEB_DIR, exist_ok=True)
    os.system(f"cp {TEST_FILE} {WEB_DIR}/")

    handler = SimpleHTTPRequestHandler
    os.chdir(WEB_DIR)
    httpd = socketserver.TCPServer(("", PORT), handler)
    httpd.serve_forever()

# Run a command and return True if successful, False otherwise
def run_command(command):
    try:
        subprocess.check_call(command, shell=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {command}")
        print(f"Error: {e}")
        return False

# Compare two files and return True if they are the same, False otherwise
def compare_files(file1, file2):
    return filecmp.cmp(file1, file2, shallow=False)

def main():
    # Check if wurl exists
    if not os.path.isfile(WURL_EXECUTABLE):
        print(f"Error: {WURL_EXECUTABLE} does not exist.")
        sys.exit(1)

    # Start the HTTP server in a separate thread
    server_thread = threading.Thread(target=start_http_server)
    server_thread.daemon = True
    server_thread.start()

    # Give the server a moment to start
    time.sleep(1)

    # Test 1: Basic download
    print("Test 1: Basic download")
    if run_command(f"{WURL_EXECUTABLE} -O {WURL_OUTPUT} {TEST_URL}") and \
       run_command(f"wget -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 1: PASS")
        else:
            print("Test 1: FAIL")
    else:
        print("Test 1: FAIL")

    # Test 2: Resume download
    print("Test 2: Resume download")
    if run_command(f"{WURL_EXECUTABLE} -c -O {WURL_OUTPUT} {TEST_URL}") and \
       run_command(f"wget -c -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 2: PASS")
        else:
            print("Test 2: FAIL")
    else:
        print("Test 2: FAIL")

    # Test 3: Follow redirects
    print("Test 3: Follow redirects")
    if run_command(f"{WURL_EXECUTABLE} -L -O {WURL_OUTPUT} {TEST_URL}") and \
       run_command(f"wget -L -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 3: PASS")
        else:
            print("Test 3: FAIL")
    else:
        print("Test 3: FAIL")

    # Clean up
    if os.path.exists(WURL_OUTPUT):
        os.remove(WURL_OUTPUT)
    if os.path.exists(WGET_OUTPUT):
        os.remove(WGET_OUTPUT)
    if os.path.exists(WEB_DIR):
        os.system(f"rm -r {WEB_DIR}")

if __name__ == "__main__":
    main()
