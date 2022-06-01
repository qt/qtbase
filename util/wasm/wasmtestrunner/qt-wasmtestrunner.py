#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import re
import os
import sys
import time
import atexit
import threading
import subprocess
import http.server
from pathlib import Path

from selenium import webdriver
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities
from selenium.webdriver.chrome.service import Service
import argparse


class WasmTestRunner:
    def __init__(self, args: dict):
        self.server_process = None
        self.browser_process = None
        self.python_path = Path(sys.executable)
        self.script_dir = Path(os.path.dirname(os.path.realpath(__file__)))
        self.host = 'localhost'
        self.webserver = None
        self.webthread = None
        self.exit_code = 0

        paths = ['html_path', 'browser_path', 'chromedriver_path', 'tmp_dir']

        for key, value in args.items():
            if value is None:
                continue
            if key in paths:
                value = Path(value)
                value.resolve()
            setattr(self, key, value)

        if not self.html_path.exists():
            raise FileNotFoundError(self.html_path)

        self.webroot = self.html_path.parent

        if hasattr(self, 'browser_path') and not self.browser_path.exists():
            raise FileNotFoundError(self.browser_path)

        atexit.register(self.cleanup)

    def run(self):
        # self.run_webserver()
        self.run_threaded_webserver()

        if self.use_browser:
            self.run_wasm_browser()
        else:
            self.run_wasm_webdriver()

        self.shutdown_threaded_webserver()

        return self.exit_code

    def run_webserver(self):
        webroot = self.html_path.parent.resolve()
        self.server_process =\
            subprocess.Popen([
                str(self.python_path),
                '-m', 'http.server',
                '--directory', str(webroot),
                self.port
            ])

    def run_threaded_webserver(self):
        self.webserver = http.server.ThreadingHTTPServer(
            (self.host, int(self.port)), self.get_http_handler_class())

        self.webthread = threading.Thread(target=self.webserver.serve_forever)
        self.webthread.start()

    def shutdown_threaded_webserver(self):
        if self.webserver is not None:
            self.webserver.shutdown()
        if self.webthread is not None:
            self.webthread.join()

    def run_wasm_webdriver(self):
        url = f'http://localhost:{self.port}/{self.html_path.name}'

        d = DesiredCapabilities.CHROME
        d['goog:loggingPrefs'] = {'browser': 'ALL'}
        ser = Service(executable_path=self.chromedriver_path)
        driver = webdriver.Chrome(desired_capabilities=d, service=ser)
        driver.get(url)

        app_state = ''

        while app_state != 'Exited':
            # HACK: Optimally, we would want the program to report back to us
            # when it changes state and prints logs
            # Unfortunately, that's rather difficult, so we resort to polling it
            # at a given interval instead, which is adjustable
            time.sleep(1)
            app_state = self.get_loader_variable(driver, 'status')

            for entry in driver.get_log('browser'):
                regex = re.compile(r'[^"]*"(.*)".*')
                match = regex.match(entry['message'])

                if match is not None:
                    console_line = match.group(1)
                    print(console_line)

        if self.get_loader_variable(driver, 'crashed'):
            self.exit_code = 1

    def run_wasm_browser(self):
        if not hasattr(self, 'browser_path'):
            print('Error: browser path must be set to run with browser')
            return

        if not hasattr(self, 'tmp_dir'):
            print('Error: tmp_dir must be set to run with browser')
            return

        self.create_tmp_dir()
        self.browser_process =\
            subprocess.Popen([
                str(self.browser_path),
                '--user-data-dir=' + str(self.tmp_dir),
                '--enable-logging=stderr',
                f'http://localhost:{self.port}/{self.html_path.name}'
            ],
                stderr=subprocess.PIPE
            )

        # Only capture the console content
        regex = re.compile(r'[^"]*CONSOLE[^"]*"(.*)"[.\w]*')

        for line in self.browser_process.stderr:
            str_line = line.decode('utf-8')

            match = regex.match(str_line)

            # Error condition, this should have matched
            if 'CONSOLE' in str_line and match is None:
                print('Error: did not match console line:')
                print(str_line)

            if match is not None:
                console_line = match.group(1)
                print(console_line)

            if 'Finished testing' in str_line:
                self.browser_process.kill()
                break

    @staticmethod
    def get_loader_variable(driver, varname: str):
        return driver.execute_script('return qtLoader.' + varname)

    def create_tmp_dir(self):
        if not self.tmp_dir.exists():
            self.tmp_dir.mkdir()

        if not self.tmp_dir.is_dir():
            raise NotADirectoryError(self.tmp_dir)

        # Needed to bypass the "Welcome to Chrome" prompt
        first_run = Path(self.tmp_dir, 'First Run')
        first_run.touch()

    def get_http_handler_class(self):
        wtr = self

        class OriginIsolationHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, request, client_address, server):
                super().__init__(request, client_address, server, directory=wtr.webroot)

            # Headers required to enable SharedArrayBuffer
            # See https://web.dev/cross-origin-isolation-guide/
            def end_headers(self):
                self.send_header("Cross-Origin-Opener-Policy", "same-origin")
                self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
                self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
                http.server.SimpleHTTPRequestHandler.end_headers(self)

            # We usually don't care that much about what the webserver is logging
            def log_message(self, format_, *args):
                return

        return OriginIsolationHTTPRequestHandler

    def cleanup(self):
        if self.browser_process is not None:
            self.browser_process.kill()
        if self.server_process is not None:
            self.server_process.kill()
        self.shutdown_threaded_webserver()


def main():
    parser = argparse.ArgumentParser(description='WASM testrunner')
    parser.add_argument('html_path', help='Path to the HTML file to request')
    parser.add_argument('--port', help='Port to run the webserver on', default='8000')
    parser.add_argument('--use_browser', action='store_true')
    parser.add_argument('--browser_path', help='Path to the browser to use')
    parser.add_argument('--chromedriver_path', help='Absolute path to chromedriver',
                        default='chromedriver')
    parser.add_argument('--tmp_dir', help='Path to the tmpdir to use when using a browser',
                        default='/tmp/wasm-testrunner')

    args = vars(parser.parse_args())

    test_runner = WasmTestRunner(args)
    return test_runner.run()


if __name__ == '__main__':
    sys.exit(main())
