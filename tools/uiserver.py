import atexit
import os
import sys
import subprocess
import http.server
import socket
import random

UI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'webui-dist')
TRACE_SYMFILE = 'result.json'

# TODO: serve trace file independently, need to change ui to fetch when loading
class TraceProcessorProcess:
    trace_processor_path = os.path.join(UI_PATH, 'trace_processor')
    
    def start(self, path):
        self.path = path
        self._process = subprocess.Popen(
            [
                sys.executable,
                self.trace_processor_path,
                self.path,
                "-D",
            ],
            stderr=subprocess.PIPE,
        )
        atexit.register(self.stop)
        self._wait_load()

    def _wait_load(self):
        print("Loading and parsing trace data, this could take a while...")
        assert self._process.stderr is not None
        while True:
            line = self._process.stderr.readline().decode("utf-8")
            if "This server can be used" in line:
                break

    def stop(self):
        self._process.terminate()
        try:
            self._process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self._process.kill()
        atexit.unregister(self.stop)


def find_free_port(start=8000, max_attempts=100):
    """Find a free TCP port."""
    for _ in range(max_attempts):
        port = random.randint(start, 49151)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if s.connect_ex(('localhost', port)) != 0:
                return port
    raise RuntimeError("No free port found.")

def start_ui(port = None):
    """Start Perfetto UI"""
    UI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'webui-dist')
    os.chdir(UI_PATH)
    if not port:
        trace_processor = TraceProcessorProcess()
        trace_processor.start('./result.json')
        # port = find_free_port()
        port = 10000 # Perfetto trace processor only accepts requests from localhost:10000
    handler = http.server.SimpleHTTPRequestHandler
    httpd = http.server.HTTPServer(('localhost', port), handler)
    print(f"🌐 VizCCTracer running at http://localhost:{port}")
    httpd.serve_forever()