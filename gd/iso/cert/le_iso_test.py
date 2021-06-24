#
#   Copyright 2021 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from cert.gd_base_test import GdBaseTestClass
from iso.cert.le_iso_test_lib import LeIsoTestBase


class LeIsoTest(GdBaseTestClass, LeIsoTestBase):

    def setup_class(self):
        GdBaseTestClass.setup_class(self, dut_module='L2CAP', cert_module='HCI_INTERFACES')

    def setup_test(self):
        GdBaseTestClass.setup_test(self)
        LeIsoTestBase.setup_test(self, self.dut, self.cert)

    def teardown_test(self):
        LeIsoTestBase.teardown_test(self)
        GdBaseTestClass.teardown_test(self)
