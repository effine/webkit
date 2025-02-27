# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import platform
import sys
import unittest2 as unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.runtests import RunTests

class RunTestsTest(unittest.TestCase):
    def test_webkit_run_unit_tests(self):
        tool = MockTool(log_executive=True)
        tool._port.run_python_unittests_command = lambda: None
        tool._port.run_perl_unittests_command = lambda: None
        step = RunTests(tool, MockOptions(test=True, non_interactive=True, quiet=False))

        if sys.platform != "cygwin":
            expected_logs = """Running bindings generation tests
MOCK run_and_throw_if_fail: ['mock-run-bindings-tests'], cwd=/mock-checkout
Running WebKit unit tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-unit-tests'], cwd=/mock-checkout
Running run-webkit-tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-tests', '--no-new-test-results', '--no-show-results', '--exit-after-n-failures=30', '--quiet', '--skip-failing-tests'], cwd=/mock-checkout
"""
        else:
            expected_logs = """Running bindings generation tests
MOCK run_and_throw_if_fail: ['mock-run-bindings-tests'], cwd=/mock-checkout
Running WebKit unit tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-unit-tests'], cwd=/mock-checkout
Running run-webkit-tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-tests', '--no-new-test-results', '--no-show-results', '--exit-after-n-failures=30', '--no-build'], cwd=/mock-checkout
"""

        OutputCapture().assert_outputs(self, step.run, [{}], expected_logs=expected_logs)
