#!/usr/bin/env python3
#
#   Copyright 2020 - The Android Open Source Project
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

from blueberry.tests.gd.cert import gd_base_test
from hci.cert.acl_manager_test_lib import AclManagerTestBase
from mobly import test_runner


class AclManagerTestBb(gd_base_test.GdBaseTestClass, AclManagerTestBase):

    def setup_class(self):
        gd_base_test.GdBaseTestClass.setup_class(self, dut_module='HCI_INTERFACES', cert_module='HCI')

    # todo: move into GdBaseTestClass, based on modules inited
    def setup_test(self):
        gd_base_test.GdBaseTestClass.setup_test(self)
        AclManagerTestBase.setup_test(self, self.dut, self.cert)

    def teardown_test(self):
        AclManagerTestBase.teardown_test(self)
        gd_base_test.GdBaseTestClass.teardown_test(self)

    def test_dut_connects(self):
        AclManagerTestBase.test_dut_connects(self)

    def test_cert_connects(self):
        AclManagerTestBase.test_cert_connects(self)

    def test_reject_broadcast(self):
        AclManagerTestBase.test_reject_broadcast(self)

    def test_cert_connects_disconnects(self):
        AclManagerTestBase.test_cert_connects_disconnects(self)

    def test_recombination_l2cap_packet(self):
        AclManagerTestBase.test_recombination_l2cap_packet(self)


if __name__ == '__main__':
    test_runner.main()
