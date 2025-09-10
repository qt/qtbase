#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import argparse
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait
from selenium import webdriver
from pathlib import Path
import typing
import http.server
import subprocess
import threading
import psutil
import re
import os
from signal import SIGINT

import sys


class StdoutOutputSink(object):
    def __init__(self):
        pass

    def write(self, data: str):
        print(data)

    def __enter__(self):
        pass

    def __exit__(self, _, __, ___):
        pass


class FileOutputSink(object):
    def __init__(self, filename: str):
        self.__filename = filename
        self.__file = None

    def write(self, data: str):
        self.__file.write(data)

    def __enter__(self):
        self.__file = open(self.__filename, 'w')

    def __exit__(self, _, __, ___):
        self.__file.close()


class OutputMulticast(object):
    def __init__(self, destinations: typing.List[str]):
        self.__sinks: typing.List[typing.Union[StdoutOutputSink, FileOutputSink]] = [
        ]
        self.__destinations = [
            'stdout'] if destinations is None else destinations
        number_of_stdout_sinks = sum(
            [1 if destination == 'stdout' else 0 for destination in self.__destinations])
        if number_of_stdout_sinks > 1:
            raise Exception('Maximum allowed number of stdout sinks is 1')

    def write(self, data: str):
        for sink in self.__sinks:
            sink.write(data)

    def _makeSink(self, destination: str):
        return StdoutOutputSink() if 'stdout' == destination else FileOutputSink(destination)

    def __enter__(self):
        for destination in self.__destinations:
            sink = self._makeSink(destination)
            sink.__enter__()
            self.__sinks.append(sink)
        return self

    def __exit__(self, _, __, ___):
        for sink in reversed(self.__sinks):
            sink.__exit__(_, __, ___)


class WasmTestRunner:
    def __init__(self, args: dict):
        self.server_process = None
        self.browser_process = None
        self.python_path = Path(sys.executable)
        self.script_dir = Path(os.path.dirname(os.path.realpath(__file__)))
        self.host = 'localhost'
        self.webserver = None
        self.webthread = None

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

    def run(self):
        self.run_threaded_webserver()

        with OutputMulticast(
                self.output if hasattr(self, 'output') else ['stdout']) as output_multicast:
            try:
                if self.use_browser:
                    return self.run_wasm_browser()
                else:
                    return self.run_wasm_webdriver(output_multicast)
            finally:
                self.cleanup()

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

    def run_wasm_webdriver(self, output_multicast: OutputMulticast):
        url = f'http://localhost:{self.port}/{self.html_path.name}'
        if (self.batched_test is not None):
            url = f'{url}?qtestname={self.batched_test}&qtestoutputformat={self.format}'

        d = DesiredCapabilities.CHROME
        d['goog:loggingPrefs'] = {'browser': 'ALL'}
        ser = Service(executable_path=self.chromedriver_path)
        driver = webdriver.Chrome(desired_capabilities=d, service=ser)
        driver.get(url)
        driver.execute_script(
            """ const status = qtTestRunner.status;
                const onFinished = status => {
                    if (status === 'Completed' || status === 'Error')
                        document.title = 'qtFinished';
                };
                onFinished(status);
                qtTestRunner.onStatusChanged.addEventListener(onFinished);
            """)

        WebDriverWait(driver, self.timeout).until(
            expected_conditions.title_is('qtFinished'))

        runner_status = driver.execute_script(f"return qtTestRunner.status")
        if runner_status == 'Error':
            output_multicast.write(driver.execute_script(
                "return qtTestRunner.errorDetails"))
            return -1
        else:
            assert runner_status == 'Completed'
            output_multicast.write(driver.execute_script(
                f"return qtTestRunner.results.get('{self.batched_test}').textOutput"))
            return driver.execute_script(
                f"return qtTestRunner.results.get('{self.batched_test}').exitCode")

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
                self.send_header(
                    "Cross-Origin-Embedder-Policy", "require-corp")
                self.send_header(
                    "Cross-Origin-Resource-Policy", "cross-origin")
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


class BackendProcess:
    def __init__(self) -> None:
        self.__process = subprocess.Popen(
            [sys.executable, *sys.argv, '--backend'], shell=False, stdout=subprocess.PIPE)

    def abort(self):
        current_process = psutil.Process(self.__process.pid)
        children = current_process.children(recursive=True)
        for child in [*children, current_process]:
            os.kill(child.pid, SIGINT)

    def communicate(self, timeout):
        return self.__process.communicate(timeout)[0].decode('utf-8')

    def returncode(self):
        return self.__process.returncode


def main():
    parser = argparse.ArgumentParser(description='WASM testrunner')
    parser.add_argument('html_path', help='Path to the HTML file to request')
    parser.add_argument(
        '--batched_test', help='Specifies a batched test to run')
    parser.add_argument('--timeout', help='Test timeout',
                        type=int, default=120)
    parser.add_argument(
        '--port', help='Port to run the webserver on', default='8000')
    parser.add_argument('--use_browser', action='store_true')
    parser.add_argument('--browser_path', help='Path to the browser to use')
    parser.add_argument('--chromedriver_path', help='Absolute path to chromedriver',
                        default='chromedriver')
    parser.add_argument('--tmp_dir', help='Path to the tmpdir to use when using a browser',
                        default='/tmp/wasm-testrunner')
    parser.add_argument(
        '-o', help='filename. Filename may be "stdout" to write to stdout.',
        action='append', dest='output')
    parser.add_argument(
        '--format', help='Output format', choices=['txt', 'xml', 'lightxml', 'junitxml', 'tap'],
        default='txt')
    parser.add_argument(
        '--backend', help='Run as a backend process. There are two types of test runner processes - '
        'the main monitoring process and the backend processes launched by it. The tests are '
        'run on the backend to avoid any undesired behavior, like deadlocks in browser main process, '
        'spilling over across test cases.',
        action='store_true')

    args = parser.parse_args()
    if not args.backend:
        backend_process = BackendProcess()
        try:
            stdout = backend_process.communicate(args.timeout)
            print(stdout)
            return backend_process.returncode()
        except Exception as e:
            print(f"Exception while executing test {e}")
            backend_process.abort()
            return -1

    return WasmTestRunner(vars(args)).run()


if __name__ == '__main__':
    sys.exit(main())
