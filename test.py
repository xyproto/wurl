import os
import sys
import subprocess
import http.server
import socketserver
import threading
import time
import filecmp
import signal
import socket


def find_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", 0))
        return s.getsockname()[1]


PORT = find_free_port()
TEST_FILE = "LICENSE"
TEST_URL = f"http://localhost:{PORT}/{TEST_FILE}"
WURL_EXECUTABLE = os.path.abspath("./wget")
WURL_OUTPUT = "/tmp/wurl.txt"
WGET_OUTPUT = "/tmp/wget.txt"
WEB_DIR = "/tmp/webserver"


class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass


def start_http_server():
    os.makedirs(WEB_DIR, exist_ok=True)
    os.system(f"cp {TEST_FILE} {WEB_DIR}/")
    handler = SimpleHTTPRequestHandler
    os.chdir(WEB_DIR)
    try:
        httpd = socketserver.TCPServer(("", PORT), handler)
        httpd.serve_forever()
    except OSError as e:
        print(f"Error: Could not start HTTP server on port {PORT}: {e}")
        sys.exit(1)


def run_command(command):
    try:
        subprocess.check_call(command, shell=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {command}")
        print(f"Error: {e}")
        return False


def compare_files(file1, file2):
    return filecmp.cmp(file1, file2, shallow=False)


def cleanup():
    if os.path.exists(WURL_OUTPUT):
        os.remove(WURL_OUTPUT)
    if os.path.exists(WGET_OUTPUT):
        os.remove(WGET_OUTPUT)
    if os.path.exists(WEB_DIR):
        os.system(f"rm -r {WEB_DIR}")


def handle_interrupt(signum, frame):
    print("\nInterrupt received, cleaning up...")
    cleanup()
    sys.exit(0)


def main():
    signal.signal(signal.SIGINT, handle_interrupt)

    if not os.path.isfile(WURL_EXECUTABLE):
        print(f"Error: {WURL_EXECUTABLE} does not exist.")
        sys.exit(1)

    server_thread = threading.Thread(target=start_http_server)
    server_thread.daemon = True
    server_thread.start()
    time.sleep(1)

    # Test 1: Basic download
    print("Test 1: Basic download")
    if run_command(f"{WURL_EXECUTABLE} -O {WURL_OUTPUT} {TEST_URL}") and run_command(
        f"wget -O {WGET_OUTPUT} {TEST_URL}"
    ):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 1: PASS")
        else:
            print("Test 1: FAIL")
    else:
        print("Test 1: FAIL")

    # Test 2: Resume download
    print("Test 2: Resume download")
    if run_command(f"{WURL_EXECUTABLE} -c -O {WURL_OUTPUT} {TEST_URL}") and run_command(
        f"wget -c -O {WGET_OUTPUT} {TEST_URL}"
    ):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 2: PASS")
        else:
            print("Test 2: FAIL")
    else:
        print("Test 2: FAIL")

    # Test 3: Follow redirects
    print("Test 3: Follow redirects")
    if run_command(f"{WURL_EXECUTABLE} -L -O {WURL_OUTPUT} {TEST_URL}") and run_command(
        f"wget -L -O {WGET_OUTPUT} {TEST_URL}"
    ):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 3: PASS")
        else:
            print("Test 3: FAIL")
    else:
        print("Test 3: FAIL")

    # Test 4: Custom User-Agent
    print("Test 4: Custom User-Agent")
    if run_command(
        f"{WURL_EXECUTABLE} --user-agent='WurlTestAgent' -O {WURL_OUTPUT} {TEST_URL}"
    ) and run_command(f"wget --user-agent='WurlTestAgent' -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 4: PASS")
        else:
            print("Test 4: FAIL")
    else:
        print("Test 4: FAIL")

    # Test 5: Set timeout
    print("Test 5: Set timeout")
    if run_command(
        f"{WURL_EXECUTABLE} --timeout=5 -O {WURL_OUTPUT} {TEST_URL}"
    ) and run_command(f"wget --timeout=5 -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 5: PASS")
        else:
            print("Test 5: FAIL")
    else:
        print("Test 5: FAIL")

    # Test 6: Limit download rate
    print("Test 6: Limit download rate")
    if run_command(
        f"{WURL_EXECUTABLE} --limit-rate=10K -O {WURL_OUTPUT} {TEST_URL}"
    ) and run_command(f"wget --limit-rate=10K -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 6: PASS")
        else:
            print("Test 6: FAIL")
    else:
        print("Test 6: FAIL")

    # Test 7: IPv4-only
    print("Test 7: IPv4-only")
    if run_command(f"{WURL_EXECUTABLE} -4 -O {WURL_OUTPUT} {TEST_URL}") and run_command(
        f"wget -4 -O {WGET_OUTPUT} {TEST_URL}"
    ):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 7: PASS")
        else:
            print("Test 7: FAIL")
    else:
        print("Test 7: FAIL")

    # Test 8: Custom Header
    print("Test 8: Custom Header")
    custom_header = "X-Test-Header: WurlTest"
    if run_command(
        f"{WURL_EXECUTABLE} --header='{custom_header}' -O {WURL_OUTPUT} {TEST_URL}"
    ) and run_command(f"wget --header='{custom_header}' -O {WGET_OUTPUT} {TEST_URL}"):
        if compare_files(WURL_OUTPUT, WGET_OUTPUT):
            print("Test 8: PASS")
        else:
            print("Test 8: FAIL")
    else:
        print("Test 8: FAIL")

    cleanup()


if __name__ == "__main__":
    main()
