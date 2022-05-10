#!/usr/bin/env python3
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""
This is a helper script that takes care of reapplying special case
modifications when regenerating a CMakeLists.txt file using
pro2cmake.py or configure.cmake with configurejson2cmake.py.

It has two modes of operation:
1) Dumb "special case" block removal and re-application.
2) Smart "special case" diff application, using a previously generated
   "clean" CMakeLists.txt/configure.cmake as a source. "clean" in this
   case means a generated file which has no "special case" modifications.

Both modes use a temporary git repository to compute and reapply
"special case" diffs.

For the first mode to work, the developer has to mark changes
with "# special case" markers on every line they want to keep. Or
enclose blocks of code they want to keep between "# special case begin"
and "# special case end" markers.

For example:

SOURCES
  foo.cpp
  bar.cpp # special case

SOURCES
  foo1.cpp
  foo2.cpp
 # special case begin
  foo3.cpp
  foo4.cpp
 # special case end

The second mode, as mentioned, requires a previous "clean"
CMakeLists.txt/configure.cmake file.

The script can then compute the exact diff between
a "clean" and "modified" (with special cases) file, and reapply that
diff to a newly generated "CMakeLists.txt"/"configure.cmake" file.

This implies that we always have to keep a "clean" file alongside the
"modified" project file for each project (corelib, gui, etc.) So we
have to commit both files to the repository.

If there is no such "clean" file, we can use the first operation mode
to generate one. After that, we only have to use the second operation
mode for the project file in question.

When the script is used, the developer only has to take care of fixing
the newly generated "modified" file. The "clean" file is automatically
handled and git add'ed by the script, and will be committed together
with the "modified" file.


"""

import re
import os
import subprocess
import filecmp
import time
import typing
import stat

from shutil import copyfile
from shutil import rmtree
from textwrap import dedent


def remove_special_cases(original: str) -> str:
    # Remove content between the following markers
    # '# special case begin' and '# special case end'.
    # This also remove the markers.
    replaced = re.sub(
        r"\n[^#\n]*?#[^\n]*?special case begin.*?#[^\n]*special case end[^\n]*?\n",
        "\n",
        original,
        0,
        re.DOTALL,
    )

    # Remove individual lines that have the "# special case" marker.
    replaced = re.sub(r"\n.*#.*special case[^\n]*\n", "\n", replaced)
    return replaced


def read_content_from_file(file_path: str) -> str:
    with open(file_path, "r") as file_fd:
        content = file_fd.read()
        return content


def write_content_to_file(file_path: str, content: str) -> None:
    with open(file_path, "w") as file_fd:
        file_fd.write(content)


def resolve_simple_git_conflicts(file_path: str, debug=False) -> None:
    content = read_content_from_file(file_path)
    # If the conflict represents the addition of a new content hunk,
    # keep the content and remove the conflict markers.
    if debug:
        print("Resolving simple conflicts automatically.")
    replaced = re.sub(r"\n<<<<<<< HEAD\n=======(.+?)>>>>>>> master\n", r"\1", content, 0, re.DOTALL)
    write_content_to_file(file_path, replaced)


def copyfile_log(src: str, dst: str, debug=False):
    if debug:
        print(f"Copying {src} to {dst}.")
    copyfile(src, dst)


def check_if_git_in_path() -> bool:
    is_win = os.name == "nt"
    for path in os.environ["PATH"].split(os.pathsep):
        git_path = os.path.join(path, "git")
        if is_win:
            git_path += ".exe"
        if os.path.isfile(git_path) and os.access(git_path, os.X_OK):
            return True
    return False


def run_process_quiet(args_string: str, debug=False) -> bool:
    if debug:
        print(f'Running command: "{args_string}"')
    args_list = args_string.split()
    try:
        subprocess.run(args_list, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # git merge with conflicts returns with exit code 1, but that's not
        # an error for us.
        if "git merge" not in args_string:
            if debug:
                print(
                    dedent(
                        f"""\
                             Error while running: "{args_string}"
                             {e.stdout}"""
                    )
                )
            return False
    return True


def does_file_have_conflict_markers(file_path: str, debug=False) -> bool:
    if debug:
        print(f"Checking if {file_path} has no leftover conflict markers.")
    content_actual = read_content_from_file(file_path)
    if "<<<<<<< HEAD" in content_actual:
        print(f"Conflict markers found in {file_path}. " "Please remove or solve them first.")
        return True
    return False


def create_file_with_no_special_cases(
    original_file_path: str, no_special_cases_file_path: str, debug=False
):
    """
    Reads content of original CMakeLists.txt/configure.cmake, removes all content
    between "# special case" markers or lines, saves the result into a
    new file.
    """
    content_actual = read_content_from_file(original_file_path)
    if debug:
        print(f"Removing special case blocks from {original_file_path}.")
    content_no_special_cases = remove_special_cases(content_actual)

    if debug:
        print(
            f"Saving original contents of {original_file_path} "
            f"with removed special case blocks to {no_special_cases_file_path}"
        )
    write_content_to_file(no_special_cases_file_path, content_no_special_cases)


def rm_tree_on_error_handler(func: typing.Callable[..., None], path: str, exception_info: tuple):
    # If the path is read only, try to make it writable, and try
    # to remove the path again.
    if not os.access(path, os.W_OK):
        os.chmod(path, stat.S_IWRITE)
        func(path)
    else:
        print(f"Error while trying to remove path: {path}. Exception: {exception_info}")


class SpecialCaseHandler(object):
    def __init__(
        self,
        original_file_path: str,
        generated_file_path: str,
        base_dir: str,
        keep_temporary_files=False,
        debug=False,
    ) -> None:
        self.base_dir = base_dir
        self.original_file_path = original_file_path
        self.generated_file_path = generated_file_path
        self.keep_temporary_files = keep_temporary_files
        self.use_heuristic = False
        self.debug = debug

    @property
    def prev_file_path(self) -> str:
        filename = ".prev_" + os.path.basename(self.original_file_path)
        return os.path.join(self.base_dir, filename)

    @property
    def post_merge_file_path(self) -> str:
        original_file_name = os.path.basename(self.original_file_path)
        (original_file_basename, original_file_ext) = os.path.splitext(original_file_name)
        filename = original_file_basename + "-post-merge" + original_file_ext
        return os.path.join(self.base_dir, filename)

    @property
    def no_special_file_path(self) -> str:
        original_file_name = os.path.basename(self.original_file_path)
        (original_file_basename, original_file_ext) = os.path.splitext(original_file_name)
        filename = original_file_basename + ".no-special" + original_file_ext
        return os.path.join(self.base_dir, filename)

    def apply_git_merge_magic(self, no_special_cases_file_path: str) -> None:
        # Create new folder for temporary repo, and ch dir into it.
        repo = os.path.join(self.base_dir, "tmp_repo")
        repo_absolute_path = os.path.abspath(repo)
        txt = os.path.basename(self.original_file_path)

        try:
            os.mkdir(repo)
            current_dir = os.getcwd()
            os.chdir(repo)
        except Exception as e:
            print(f"Failed to create temporary directory for temporary git repo. Exception: {e}")
            raise e

        generated_file_path = os.path.join("..", self.generated_file_path)
        original_file_path = os.path.join("..", self.original_file_path)
        no_special_cases_file_path = os.path.join("..", no_special_cases_file_path)
        post_merge_file_path = os.path.join("..", self.post_merge_file_path)

        try:
            # Create new repo with the "clean" CMakeLists.txt/configure.cmake file.
            run_process_quiet("git init .", debug=self.debug)
            run_process_quiet("git config user.name fake", debug=self.debug)
            run_process_quiet("git config user.email fake@fake", debug=self.debug)
            copyfile_log(no_special_cases_file_path, txt, debug=self.debug)
            run_process_quiet(f"git add {txt}", debug=self.debug)
            run_process_quiet("git commit -m no_special", debug=self.debug)
            run_process_quiet("git checkout -b no_special", debug=self.debug)

            # Copy the original "modified" file (with the special cases)
            # and make a new commit.
            run_process_quiet("git checkout -b original", debug=self.debug)
            copyfile_log(original_file_path, txt, debug=self.debug)
            run_process_quiet(f"git add {txt}", debug=self.debug)
            run_process_quiet("git commit -m original", debug=self.debug)

            # Checkout the commit with "clean" file again, and create a
            # new branch.
            run_process_quiet("git checkout no_special", debug=self.debug)
            run_process_quiet("git checkout -b newly_generated", debug=self.debug)

            # Copy the new "modified" file and make a commit.
            copyfile_log(generated_file_path, txt, debug=self.debug)
            run_process_quiet(f"git add {txt}", debug=self.debug)
            run_process_quiet("git commit -m newly_generated", debug=self.debug)

            # Merge the "old" branch with modifications into the "new"
            # branch with the newly generated file.
            run_process_quiet("git merge original", debug=self.debug)

            # Resolve some simple conflicts (just remove the markers)
            # for cases that don't need intervention.
            resolve_simple_git_conflicts(txt, debug=self.debug)

            # Copy the resulting file from the merge.
            copyfile_log(txt, post_merge_file_path)
        except Exception as e:
            print(f"Git merge conflict resolution process failed. Exception: {e}")
            raise e
        finally:
            os.chdir(current_dir)

            # Remove the temporary repo.
            try:
                if not self.keep_temporary_files:
                    rmtree(repo_absolute_path, onerror=rm_tree_on_error_handler)
            except Exception as e:
                print(f"Error removing temporary repo. Exception: {e}")

    def save_next_clean_file(self):
        files_are_equivalent = filecmp.cmp(self.generated_file_path, self.post_merge_file_path)

        if not files_are_equivalent:
            # Before overriding the generated file with the post
            # merge result, save the new "clean" file for future
            # regenerations.
            copyfile_log(self.generated_file_path, self.prev_file_path, debug=self.debug)

            # Attempt to git add until we succeed. It can fail when
            # run_pro2cmake executes pro2cmake in multiple threads, and git
            # has acquired the index lock.
            success = False
            failed_once = False
            i = 0
            while not success and i < 20:
                success = run_process_quiet(f"git add {self.prev_file_path}", debug=self.debug)
                if not success:
                    failed_once = True
                    i += 1
                    time.sleep(0.1)

                if failed_once and not success:
                    if self.debug:
                        print("Retrying git add, the index.lock was probably acquired.")
            if failed_once and success:
                if self.debug:
                    print("git add succeeded.")
            elif failed_once and not success:
                print(f"git add failed. Make sure to git add {self.prev_file_path} yourself.")

    def handle_special_cases_helper(self) -> bool:
        """
        Uses git to reapply special case modifications to the "new"
        generated CMakeLists.gen.txt/configure.cmake.gen file.

        If use_heuristic is True, a new file is created from the
        original file, with special cases removed.

        If use_heuristic is False, an existing "clean" file with no
        special cases is used from a previous conversion. The "clean"
        file is expected to be in the same folder as the original one.
        """
        try:
            if does_file_have_conflict_markers(self.original_file_path):
                return False

            if self.use_heuristic:
                create_file_with_no_special_cases(
                    self.original_file_path, self.no_special_file_path
                )
                no_special_cases_file_path = self.no_special_file_path
            else:
                no_special_cases_file_path = self.prev_file_path

            if self.debug:
                print(
                    f"Using git to reapply special case modifications to newly "
                    f"generated {self.generated_file_path} file"
                )

            self.apply_git_merge_magic(no_special_cases_file_path)
            self.save_next_clean_file()

            copyfile_log(self.post_merge_file_path, self.generated_file_path)
            if not self.keep_temporary_files:
                os.remove(self.post_merge_file_path)
            if self.debug:
                print(
                    "Special case reapplication using git is complete. "
                    "Make sure to fix remaining conflict markers."
                )

        except Exception as e:
            print(f"Error occurred while trying to reapply special case modifications: {e}")
            return False
        finally:
            if not self.keep_temporary_files and self.use_heuristic:
                os.remove(self.no_special_file_path)

        return True

    def handle_special_cases(self) -> bool:
        original_file_exists = os.path.isfile(self.original_file_path)
        prev_file_exists = os.path.isfile(self.prev_file_path)
        self.use_heuristic = not prev_file_exists

        git_available = check_if_git_in_path()
        keep_special_cases = original_file_exists and git_available

        if not git_available:
            print(
                "You need to have git in PATH in order to reapply the special "
                "case modifications."
            )

        copy_generated_file = True

        if keep_special_cases:
            copy_generated_file = self.handle_special_cases_helper()

        return copy_generated_file
