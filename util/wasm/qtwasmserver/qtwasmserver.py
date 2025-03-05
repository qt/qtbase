#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import argparse
import os
import pathlib
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
from enum import Enum
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from subprocess import run
from functools import partial

import brotli
import netifaces as ni
from httpcompressionserver import HTTPCompressionRequestHandler


def generate_mkcert_certificate(addresses):
    """ "Generates a https certificate for localhost and selected addresses. This
    requires that the mkcert utility is installed, and that a certificate
    authority key pair (rootCA-key.pem and rootCA.pem) has been generated. The
    certificates are written to /tmp, where the http server can find them
    ater on."""

    cert_file = tempfile.NamedTemporaryFile(
        mode="w+b", prefix="qtwasmserver-certificate-", suffix=".pem", delete=True
    )
    cert_key_file = tempfile.NamedTemporaryFile(
        mode="w+b", prefix="qtwasmserver-certificate-key-", suffix=".pem", delete=True
    )

    # check if mkcert is installed
    try:
        out = subprocess.check_output(["mkcert", "-CAROOT"])
        root_ca_path = out.decode("utf-8").strip()
        print(
            "Generating certificates with mkcert, using certificate authority files at:"
        )
        print(f"   {root_ca_path}       [from 'mkcert -CAROOT'] \n")
    except Exception as e:
        print("Warning: Unable to run mkcert. Will not start https server.")
        print(e)
        print(f"Install mkcert from github.com/FiloSottile/mkcert to fix this.\n")
        return False, None, None

    # generate certificates using mkcert
    addresses_string = f"localhost {' '.join(addresses)}"
    print("=== begin mkcert output ===\n")
    ret = run(
        f"mkcert -cert-file {cert_file.name} -key-file {cert_key_file.name} {addresses_string}",
        shell=True,
    )
    print("=== end mkcert output ===\n")
    has_certificate = ret.returncode == 0
    if not has_certificate:
        print(
            "Warning: mkcert is not installed or was unable to create a certificate. Will not start HTTPS server."
        )
    return has_certificate, cert_file, cert_key_file


def send_cross_origin_isolation_headers(handler):
    """Sends COOP and COEP cross origin isolation headers"""
    handler.send_header("Cross-Origin-Opener-Policy", "same-origin")
    handler.send_header("Cross-Origin-Embedder-Policy", "require-corp")
    handler.send_header("Cross-Origin-Resource-Policy", "cross-origin")


def send_empty_favicon(handle):
    """Sends an empty icon to surpess missing faviocon errors"""
    self.send_response(200)
    self.send_header("Content-Type", "image/x-icon")
    self.send_header("Content-Length", 0)


class HttpRequestHandler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def end_headers(self):
        if self.cross_origin_isolation == True:
            send_cross_origin_isolation_headers(self)
        super().end_headers()


class CompressionHttpRequesthandler(HTTPCompressionRequestHandler):
    protocol_version = "HTTP/1.1"

    # Make sure we compress: Add wasm and octet-stream to compressed_types
    compressed_types = HTTPCompressionRequestHandler.compressed_types.copy()
    compressed_types.append("application/wasm")
    compressed_types.append("application/octet-stream")

    def brotli_producer(fileobj):
        bufsize = 1024 * 512

        # Create compressor with quallity such that applying compression brings an actual speedup,
        # i.e. the server must compress fast enough to saturate a typical network
        # connection with compressed data. For brotli the quality goes from 0 to 11.
        compressor = brotli.Compressor(quality=2)
        with fileobj:
            while True:
                buf = fileobj.read(bufsize)
                if not buf:
                    yield compressor.finish()
                    return
                yield compressor.compress(buf)

                # must flush compressor state to work around crash/assert in brotlicffi,
                # see https://github.com/python-hyper/brotlicffi/issues/167
                buf = compressor.flush()
                if len(buf) > 0:
                    yield buf

    # Ideally, we would like to use the default gzip/deflate support as well,
    # however that makes gzip take precedence over brotli. In practice, all
    # browsers support brotli, so we can just use that.
    compressions = {}  # HTTPCompressionRequestHandler.compressions.copy()
    compressions["br"] = brotli_producer

    def end_headers(self):
        if self.cross_origin_isolation == True:
            send_cross_origin_isolation_headers(self)
        super().end_headers()


class CompressionMode(Enum):
    AUTO = "Auto"
    ALWAYS = "Always"
    NEVER = "Never"


def select_http_handler_class(compression_mode, address):

    """Returns the http handler class to use, based on the compression mode,
    and the address of the server for the auto mode."""
    if compression_mode == CompressionMode.ALWAYS:
        return CompressionHttpRequesthandler
    elif compression_mode == CompressionMode.NEVER:
        return HttpRequestHandler
    else:
        # Select http request handler based on addrees. If the address is
        # localhost then compression is typically not worth it since the
        # localhost connection is very fast. For other addresses we assume
        # typical network bandwidth and enable compression to reduce the download
        # size.
        if address == "127.0.0.1":
            return HttpRequestHandler
        else:
            return CompressionHttpRequesthandler


# Serve serve_path from http(s)://address:port, with certificates from certdir if set
def serve_on_thread(
    address,
    port,
    secure,
    cert_file,
    cert_key_file,
    compression_mode,
    cross_origin_isolation,
    serve_path,
):
    handler = select_http_handler_class(compression_mode, address)
    handler.cross_origin_isolation = cross_origin_isolation

    try:
        httpd = ThreadingHTTPServer((address, port), partial(handler, directory=serve_path))
    except Exception as e:
        print(f"\n### Error starting HTTP server: {e}\n")
        exit(1)

    if secure:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert_file.name, cert_key_file.name)
        httpd.socket = context.wrap_socket(
            httpd.socket,
            server_side=True,
        )
    thread = threading.Thread(target=httpd.serve_forever)
    thread.start()


def main():
    parser = argparse.ArgumentParser(
        description="A development web server for Qt for WebAssembly applications.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--port",
        "-p",
        help="Port on which to listen for HTTP and HTTPS (PORT + 1)",
        type=int,
        default=8000,
    )
    parser.add_argument(
        "--address",
        "-a",
        help="Bind to additional address, in addition to localhost",
        action="append",
    )
    parser.add_argument(
        "--all-interfaces",
        "-A",
        help="Bind to all local interfaces, instead of locahost only",
        action="store_true",
    )
    parser.add_argument(
        "--cross-origin-isolation",
        "-i",
        help="Enables cross-origin isolation mode, required for WebAssembly threads",
        action="store_true",
    )
    parser.add_argument(
        "--compress-auto",
        help="Enables file compression on non-localhost addresses",
        action="store_true",
        default=True,
    )
    parser.add_argument(
        "--compress-always",
        help="Enables file compression for all addresses",
        action="store_true",
    )
    parser.add_argument(
        "--compress-never",
        help="Disables file compression",
        action="store_true",
    )
    parser.add_argument(
        "path", help="The directory to serve", nargs="?", default=os.getcwd()
    )

    args = parser.parse_args()
    http_port = args.port
    https_port = http_port + 1
    all_interfaces = args.all_interfaces
    cmd_addresses = args.address or []
    serve_path = args.path
    cross_origin_isolation = args.cross_origin_isolation

    if not os.path.isdir(serve_path):
        print(f"The provided path '{serve_path}' does not exist or is not a directory")
        exit(1)

    compression_mode = CompressionMode.AUTO
    if args.compress_always:
        compression_mode = CompressionMode.ALWAYS
    elif args.compress_never:
        compression_mode = CompressionMode.NEVER

    print("Qt for WebAssembly development server.\n")
    print(f"Web server root:\n  {serve_path}\n")

    addresses = ["127.0.0.1"] + cmd_addresses
    if all_interfaces:
        addresses += [
            addr[ni.AF_INET][0]["addr"]
            for addr in map(ni.ifaddresses, ni.interfaces())
            if ni.AF_INET in addr
        ]
    addresses = list(set(addresses))  # deduplicate
    addresses.sort()

    print("Serving at addresses:")
    print(f"   {addresses}\n")

    has_certificate, cert_file, cert_key_file = generate_mkcert_certificate(addresses)

    print("Options:")
    print(f"   Secure server:          {has_certificate}")
    print(f"   Cross Origin Isolation: {cross_origin_isolation}")
    print(f"   Compression:            {compression_mode.value}")
    print("")

    # Start servers
    print(f"Serving at:")
    for address in addresses:
        print(f"    http://{address}:{http_port}")
        serve_on_thread(
            address,
            http_port,
            False,
            cert_file,
            cert_key_file,
            compression_mode,
            cross_origin_isolation,
            serve_path,
        )

    if has_certificate:
        for address in addresses:
            print(f"    https://{address}:{https_port}")
            serve_on_thread(
                address,
                https_port,
                True,
                cert_file,
                cert_key_file,
                compression_mode,
                cross_origin_isolation,
                serve_path,
            )


if __name__ == "__main__":
    main()
