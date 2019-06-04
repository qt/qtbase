#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2019 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

"""
This is a helper script that takes care of reapplying special case
modifications when regenerating a CMakeLists.txt file using
pro2cmake.py.

It has two modes of operation:
1) Dumb "special case" block removal and re-application.
2) Smart "special case" diff application, using a previously generated
   "clean" CMakeLists.txt as a source. "clean" in this case means a
   generated file which has no "special case" modifications.

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
CMakeLists.txt file.

The script can then compute the exact diff between
a "clean" and "modified" (with special cases) file, and reapply that
diff to a newly generated "CMakeLists.txt" file.

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

from shutil import copyfile
from shutil import rmtree


def remove_special_cases(original: str) -> str:
    # Remove content between the following markers
    # '# special case begin' and '# special case end'.
    # This also remove the markers.
    replaced = re.sub(r'\n[^#\n]*?#[^\n]*?special case begin.*?#[^\n]*special case end[^\n]*?\n',
                      '\n',
                      original,
                      0,
                      re.DOTALL)

    # Remove individual lines that have the "# special case" marker.
    replaced = re.sub(r'\n.*#.*special case[^\n]*\n', '\n', replaced)
    return replaced


def read_content_from_file(file_path: str) -> str:
    with open(file_path, 'r') as file_fd:
        content = file_fd.read()
        return content


def write_content_to_file(file_path: str, content: str) -> None:
    with open(file_path, 'w') as file_fd:
        file_fd.write(content)


def resolve_simple_git_conflicts(file_path: str, debug=False) -> None:
    content = read_content_from_file(file_path)
    # If the conflict represents the addition of a new content hunk,
    # keep the content and remove the conflict markers.
    if debug:
        print('Resolving simple conflicts automatically.')
    replaced = re.sub(r'\n<<<<<<< HEAD\n=======(.+?)>>>>>>> master\n', r'\1', content, 0, re.DOTALL)
    write_content_to_file(file_path, replaced)


def copyfile_log(src: str, dst: str, debug=False):
    if debug:
        print('Copying {} to {}.'.format(src, dst))
    copyfile(src, dst)


def check_if_git_in_path() -> bool:
    for path in os.environ['PATH'].split(os.pathsep):
        git_path = os.path.join(path, 'git')
        if os.path.isfile(git_path) and os.access(git_path, os.X_OK):
            return True
    return False


def run_process_quiet(args_string: str, debug=False) -> None:
    if debug:
        print('Running command: "{}\"'.format(args_string))
    args_list = args_string.split()
    try:
        subprocess.run(args_list, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # git merge with conflicts returns with exit code 1, but that's not
        # an error for us.
        if 'git merge' not in args_string:
            print('Error while running: "{}"\n{}'.format(args_string, e.stdout))


def does_file_have_conflict_markers(file_path: str, debug=False) -> bool:
    if debug:
        print('Checking if {} has no leftover conflict markers.'.format(file_path))
    content_actual = read_content_from_file(file_path)
    if '<<<<<<< HEAD' in content_actual:
        print('Conflict markers found in {}. '
              'Please remove or solve them first.'.format(file_path))
        return True
    return False


def create_file_with_no_special_cases(original_file_path: str, no_special_cases_file_path: str, debug=False):
    """
    Reads content of original CMakeLists.txt, removes all content
    between "# special case" markers or lines, saves the result into a
    new file.
    """
    content_actual = read_content_from_file(original_file_path)
    if debug:
        print('Removing special case blocks from {}.'.format(original_file_path))
    content_no_special_cases = remove_special_cases(content_actual)

    if debug:
        print('Saving original contents of {} '
              'with removed special case blocks to {}'.format(original_file_path,
                                                          no_special_cases_file_path))
    write_content_to_file(no_special_cases_file_path, content_no_special_cases)


class SpecialCaseHandler(object):

    def __init__(self,
                 original_file_path: str,
                 generated_file_path: str,
                 base_dir: str,
                 keep_temporary_files=False,
                 debug=False) -> None:
        self.base_dir = base_dir
        self.original_file_path = original_file_path
        self.generated_file_path = generated_file_path
        self.keep_temporary_files = keep_temporary_files
        self.use_heuristic = False
        self.debug = debug

    @property
    def prev_file_path(self) -> str:
        return os.path.join(self.base_dir, '.prev_CMakeLists.txt')

    @property
    def post_merge_file_path(self) -> str:
        return os.path.join(self.base_dir, 'CMakeLists-post-merge.txt')

    @property
    def no_special_file_path(self) -> str:
        return os.path.join(self.base_dir, 'CMakeLists.no-special.txt')

    def apply_git_merge_magic(self, no_special_cases_file_path: str) -> None:
        # Create new folder for temporary repo, and ch dir into it.
        repo = os.path.join(self.base_dir, 'tmp_repo')
        repo_absolute_path = os.path.abspath(repo)
        txt = 'CMakeLists.txt'

        try:
            os.mkdir(repo)
            current_dir = os.getcwd()
            os.chdir(repo)
        except Exception as e:
            print('Failed to create temporary directory for temporary git repo. Exception: {}'
                  .format(e))
            raise e

        generated_file_path = os.path.join("..", self.generated_file_path)
        original_file_path = os.path.join("..", self.original_file_path)
        no_special_cases_file_path = os.path.join("..", no_special_cases_file_path)
        post_merge_file_path = os.path.join("..", self.post_merge_file_path)

        try:
            # Create new repo with the "clean" CMakeLists.txt file.
            run_process_quiet('git init .', debug=self.debug)
            run_process_quiet('git config user.name fake', debug=self.debug)
            run_process_quiet('git config user.email fake@fake', debug=self.debug)
            copyfile_log(no_special_cases_file_path, txt, debug=self.debug)
            run_process_quiet('git add {}'.format(txt), debug=self.debug)
            run_process_quiet('git commit -m no_special', debug=self.debug)
            run_process_quiet('git checkout -b no_special', debug=self.debug)

            # Copy the original "modified" file (with the special cases)
            # and make a new commit.
            run_process_quiet('git checkout -b original', debug=self.debug)
            copyfile_log(original_file_path, txt, debug=self.debug)
            run_process_quiet('git add {}'.format(txt), debug=self.debug)
            run_process_quiet('git commit -m original', debug=self.debug)

            # Checkout the commit with "clean" file again, and create a
            # new branch.
            run_process_quiet('git checkout no_special', debug=self.debug)
            run_process_quiet('git checkout -b newly_generated', debug=self.debug)

            # Copy the new "modified" file and make a commit.
            copyfile_log(generated_file_path, txt, debug=self.debug)
            run_process_quiet('git add {}'.format(txt), debug=self.debug)
            run_process_quiet('git commit -m newly_generated', debug=self.debug)

            # Merge the "old" branch with modifications into the "new"
            # branch with the newly generated file.
            run_process_quiet('git merge original', debug=self.debug)

            # Resolve some simple conflicts (just remove the markers)
            # for cases that don't need intervention.
            resolve_simple_git_conflicts(txt, debug=self.debug)

            # Copy the resulting file from the merge.
            copyfile_log(txt, post_merge_file_path)
        except Exception as e:
            print('Git merge conflict resolution process failed. Exception: {}'.format(e))
            raise e
        finally:
            # Remove the temporary repo.
            try:
                if not self.keep_temporary_files:
                    rmtree(repo_absolute_path)
            except Exception as e:
                print(e)

            os.chdir(current_dir)

    def save_next_clean_file(self):
        files_are_equivalent = filecmp.cmp(self.generated_file_path, self.post_merge_file_path)

        if not files_are_equivalent:
            # Before overriding the generated file with the post
            # merge result, save the new "clean" file for future
            # regenerations.
            copyfile_log(self.generated_file_path, self.prev_file_path, debug=self.debug)
            run_process_quiet("git add {}".format(self.prev_file_path), debug=self.debug)

    def handle_special_cases_helper(self) -> bool:
        """
        Uses git to reapply special case modifications to the "new"
        generated CMakeLists.gen.txt file.

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
                create_file_with_no_special_cases(self.original_file_path,
                                                  self.no_special_file_path)
                no_special_cases_file_path = self.no_special_file_path
            else:
                no_special_cases_file_path = self.prev_file_path

            if self.debug:
                print('Using git to reapply special case modifications to newly generated {} '
                      'file'.format(self.generated_file_path))

            self.apply_git_merge_magic(no_special_cases_file_path)
            self.save_next_clean_file()

            copyfile_log(self.post_merge_file_path, self.generated_file_path)
            if not self.keep_temporary_files:
                os.remove(self.post_merge_file_path)

            print('Special case reapplication using git is complete. '
                  'Make sure to fix remaining conflict markers.')

        except Exception as e:
            print('Error occurred while trying to reapply special case modifications: {}'.format(e))
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
            print('You need to have git in PATH in order to reapply the special '
                  'case modifications.')

        copy_generated_file = True

        if keep_special_cases:
            copy_generated_file = self.handle_special_cases_helper()

        return copy_generated_file
