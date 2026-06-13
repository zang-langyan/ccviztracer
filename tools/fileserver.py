import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
from utils.filesource import getsource

class FileSourceHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status=200, content_type='application/json'):
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == '/ccviztracer/getfilesource':
            params = parse_qs(parsed.query)
            path = params.get('path', [''])[0]
            src = getsource(path)
            response = {'message': f'{src}'}
            self._set_headers(200)
            self.wfile.write(json.dumps(response).encode('utf-8'))
        else:
            self._set_headers(404)
            self.wfile.write(json.dumps({'error': 'Not found'}).encode('utf-8'))

def start_filesource_server():
    server = HTTPServer(('127.0.0.1', 27787), FileSourceHandler)
    # print('Server running on http://127.0.0.1:27787')
    server.serve_forever()

if __name__ == '__main__':
    start_filesource_server()