#!/usr/bin/env python3
"""
Simple HTTP backend for the Compiler Analyzer frontend.
Runs the C++ executable and returns its output.
"""

import http.server
import json
import os
import subprocess
import tempfile

PORT = 8080
EXE  = "./compiler_analyzer.exe"


class Handler(http.server.BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        pass  # suppress default request logs

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self._serve_file("index.html", "text/html")
        elif self.path == "/grammar":
            try:
                with open("samples/grammar.txt", "r") as f:
                    content = f.read()
                self._send(200, "text/plain", content.encode())
            except FileNotFoundError:
                self._send(404, "text/plain", b"grammar.txt not found")
        else:
            self._send(404, "text/plain", b"Not found")

    def do_POST(self):
        if self.path == "/run":
            length  = int(self.headers.get("Content-Length", 0))
            body    = self.rfile.read(length)
            try:
                data = json.loads(body)
            except Exception:
                self._send(400, "application/json",
                           json.dumps({"error": "Bad JSON"}).encode())
                return

            source   = data.get("source", "")
            grammar  = data.get("grammar", "")
            tokens   = data.get("tokens", "")
            module   = str(data.get("module", "0"))   # "0" = all
            verbose  = data.get("verbose", False)
            table    = data.get("table", False)

            # Write inputs to temp files
            with tempfile.NamedTemporaryFile(mode="w", suffix=".mc",
                                             delete=False, encoding="utf-8") as sf:
                sf.write(source); src_path = sf.name

            with tempfile.NamedTemporaryFile(mode="w", suffix=".txt",
                                             delete=False, encoding="utf-8") as gf:
                gf.write(grammar); gram_path = gf.name

            tok_path = None
            if tokens.strip():
                with tempfile.NamedTemporaryFile(mode="w", suffix=".txt",
                                                 delete=False, encoding="utf-8") as tf:
                    tf.write(tokens); tok_path = tf.name

            # Build command
            cmd = [EXE,
                   "--input",   src_path,
                   "--grammar", gram_path]
            if module != "0":
                cmd += ["--module", module]
            if tok_path:
                cmd += ["--tokens", tok_path]
            if verbose:
                cmd.append("--verbose")
            if table:
                cmd.append("--table")

            try:
                result = subprocess.run(cmd, capture_output=True,
                                        text=True, timeout=15)
                output = result.stdout + result.stderr
            except FileNotFoundError:
                output = "ERROR: compiler_analyzer.exe not found.\nRun: g++ -std=c++17 -O2 -o compiler_analyzer.exe $(find src -name '*.cpp')"
            except subprocess.TimeoutExpired:
                output = "ERROR: Analysis timed out (>15s)."
            finally:
                for p in [src_path, gram_path, tok_path]:
                    if p:
                        try: os.unlink(p)
                        except: pass

            self._send(200, "application/json",
                       json.dumps({"output": output}).encode())
        else:
            self._send(404, "application/json",
                       json.dumps({"error": "Not found"}).encode())

    def do_OPTIONS(self):
        self.send_response(200)
        self._cors()
        self.end_headers()

    def _cors(self):
        self.send_header("Access-Control-Allow-Origin",  "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def _serve_file(self, filename, content_type):
        try:
            with open(filename, "rb") as f:
                data = f.read()
            self._send(200, content_type, data)
        except FileNotFoundError:
            self._send(404, "text/plain", b"File not found")

    def _send(self, code, ctype, data):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self._cors()
        self.end_headers()
        self.wfile.write(data)


if __name__ == "__main__":
    print(f"Compiler Analyzer server running at http://localhost:{PORT}")
    print("Open index.html in browser OR visit http://localhost:8080")
    print("Press Ctrl+C to stop.\n")
    http.server.HTTPServer(("", PORT), Handler).serve_forever()
