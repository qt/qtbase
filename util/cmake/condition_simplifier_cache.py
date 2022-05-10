#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import atexit
import hashlib
import json
import os
import sys
import time

from typing import Any, Callable, Dict

condition_simplifier_cache_enabled = True


def set_condition_simplified_cache_enabled(value: bool):
    global condition_simplifier_cache_enabled
    condition_simplifier_cache_enabled = value


def get_current_file_path() -> str:
    try:
        this_file = __file__
    except NameError:
        this_file = sys.argv[0]
    this_file = os.path.abspath(this_file)
    return this_file


def get_cache_location() -> str:
    this_file = get_current_file_path()
    dir_path = os.path.dirname(this_file)
    cache_path = os.path.join(dir_path, ".pro2cmake_cache", "cache.json")
    return cache_path


def get_file_checksum(file_path: str) -> str:
    try:
        with open(file_path, "r") as content_file:
            content = content_file.read()
    except IOError:
        content = str(time.time())
    checksum = hashlib.md5(content.encode("utf-8")).hexdigest()
    return checksum


def get_condition_simplifier_checksum() -> str:
    current_file_path = get_current_file_path()
    dir_name = os.path.dirname(current_file_path)
    condition_simplifier_path = os.path.join(dir_name, "condition_simplifier.py")
    return get_file_checksum(condition_simplifier_path)


def init_cache_dict():
    return {
        "checksum": get_condition_simplifier_checksum(),
        "schema_version": "1",
        "cache": {"conditions": {}},
    }


def merge_dicts_recursive(a: Dict[str, Any], other: Dict[str, Any]) -> Dict[str, Any]:
    """Merges values of "other" into "a", mutates a."""
    for key in other:
        if key in a:
            if isinstance(a[key], dict) and isinstance(other[key], dict):
                merge_dicts_recursive(a[key], other[key])
            elif a[key] == other[key]:
                pass
        else:
            a[key] = other[key]
    return a


def open_file_safe(file_path: str, mode: str = "r+"):
    # Use portalocker package for file locking if available,
    # otherwise print a message to install the package.
    try:
        import portalocker  # type: ignore

        return portalocker.Lock(file_path, mode=mode, flags=portalocker.LOCK_EX)
    except ImportError:
        print(
            "The conversion script is missing a required package: portalocker. Please run "
            "python -m pip install -r requirements.txt to install the missing dependency."
        )
        exit(1)


def simplify_condition_memoize(f: Callable[[str], str]):
    cache_path = get_cache_location()
    cache_file_content: Dict[str, Any] = {}

    if os.path.exists(cache_path):
        try:
            with open_file_safe(cache_path, mode="r") as cache_file:
                cache_file_content = json.load(cache_file)
        except (IOError, ValueError):
            print(f"Invalid pro2cmake cache file found at: {cache_path}. Removing it.")
            os.remove(cache_path)

    if not cache_file_content:
        cache_file_content = init_cache_dict()

    current_checksum = get_condition_simplifier_checksum()
    if cache_file_content["checksum"] != current_checksum:
        cache_file_content = init_cache_dict()

    def update_cache_file():
        if not os.path.exists(cache_path):
            os.makedirs(os.path.dirname(cache_path), exist_ok=True)
            # Create the file if it doesn't exist, but don't override
            # it.
            with open(cache_path, "a"):
                pass

        updated_cache = cache_file_content

        with open_file_safe(cache_path, "r+") as cache_file_write_handle:
            # Read any existing cache content, and truncate the file.
            cache_file_existing_content = cache_file_write_handle.read()
            cache_file_write_handle.seek(0)
            cache_file_write_handle.truncate()

            # Merge the new cache into the old cache if it exists.
            if cache_file_existing_content:
                possible_cache = json.loads(cache_file_existing_content)
                if (
                    "checksum" in possible_cache
                    and "schema_version" in possible_cache
                    and possible_cache["checksum"] == cache_file_content["checksum"]
                    and possible_cache["schema_version"] == cache_file_content["schema_version"]
                ):
                    updated_cache = merge_dicts_recursive(dict(possible_cache), updated_cache)

            json.dump(updated_cache, cache_file_write_handle, indent=4)

            # Flush any buffered writes.
            cache_file_write_handle.flush()
            os.fsync(cache_file_write_handle.fileno())

    atexit.register(update_cache_file)

    def helper(condition: str) -> str:
        if (
            condition not in cache_file_content["cache"]["conditions"]
            or not condition_simplifier_cache_enabled
        ):
            cache_file_content["cache"]["conditions"][condition] = f(condition)
        return cache_file_content["cache"]["conditions"][condition]

    return helper
