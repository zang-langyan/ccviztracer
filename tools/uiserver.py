import os
import socketserver
from http.server import SimpleHTTPRequestHandler
import socket
import random
import json
from urllib.parse import urlparse, parse_qs
from pathlib import Path
import contextlib
from utils.filesource import getsource

UI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'webui-dist')
TRACE_SYMFILE = 'result.json'

@contextlib.contextmanager
def chdir_temp(d):
    cur = os.getcwd()
    os.chdir(d)
    try:
        yield
    finally:
        os.chdir(cur)


class TraceFileServer(SimpleHTTPRequestHandler):
    trace_file_path = None
    ui_path = UI_PATH
    sourcefile_path: str
    debug_mode = False
    def _set_headers(self, status=200, content_type='application/json'):
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        if path == "/ccviztracer/localtrace" and self.trace_file_path:
            trace_path = Path(self.trace_file_path)
            if not self.trace_file_path or not trace_path.is_file():
                self.send_error(404, "Trace file not found")
                return
            content_type = "application/octet-stream"
            if trace_path.suffix == ".json":
                content_type = "application/json"
            elif trace_path.suffix == ".gz":
                content_type = "application/gzip"
            self._set_headers(200, content_type)
            with open(trace_path, "rb") as f:
                self.wfile.write(f.read())
        elif path == '/ccviztracer/getfilesource':
            params = parse_qs(parsed.query)
            source_path = params.get('path', [''])[0]
            with chdir_temp(self.sourcefile_path):
                src = getsource(source_path)
            response = {'message': f'{src}'}
            self._set_headers(200)
            self.wfile.write(json.dumps(response).encode('utf-8'))
        else:
            return super().do_GET()

    def log_message(self, format, *args):
        if self.debug_mode:
            super().log_message(format, *args)

def find_free_port(start=8000, max_attempts=100):
    """Find a free TCP port."""
    for _ in range(max_attempts):
        port = random.randint(start, 49151)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if s.connect_ex(('localhost', port)) != 0:
                return port
    raise RuntimeError("No free port found.")

def start_ui(port = None, tracefile = None, isTest = False):
    """Start Perfetto UI"""
    if not port:
        port = find_free_port()
    TraceFileServer.trace_file_path = tracefile
    TraceFileServer.sourcefile_path = os.getcwd()
    if isTest:
        TraceFileServer.debug_mode = True
        TraceFileServer.ui_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'test-dist')
    os.chdir(TraceFileServer.ui_path)

    with socketserver.TCPServer(('127.0.0.1', port), TraceFileServer) as httpd:
        print(f"🌐 VizCCTracer running at http://127.0.0.1:{port}")
        httpd.serve_forever()