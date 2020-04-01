#!/usr/bin/env python3
#
#   Copyright 2019 - The Android Open Source Project
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

from datetime import datetime, timedelta
import logging
import time

from mobly import asserts

from acts.base_test import BaseTestClass

from bluetooth_packets_python3 import hci_packets
from bluetooth_packets_python3 import l2cap_packets
from cert.event_stream import EventStream, FilteringEventStream
from cert.truth import assertThat
from cert.metadata import metadata, MetadataKey


class BogusProto:

    class BogusType:

        def __init__(self):
            self.name = "BogusProto"
            self.is_extension = False
            self.cpp_type = False

        def type(self):
            return 'BogusRpc'

        def label(self):
            return "label"

    class BogusDescriptor:

        def __init__(self, name):
            self.full_name = name

    def __init__(self, value):
        self.value_ = value
        self.DESCRIPTOR = BogusProto.BogusDescriptor(str(value))

    def __str__(self):
        return "BogusRpc value = " + str(self.value_)

    def ListFields(self):
        for field in [BogusProto.BogusType()]:
            yield [field, self.value_]


class FetchEvents:

    def __init__(self, events, delay_ms):
        self.events_ = events
        self.sleep_time_ = (delay_ms * 1.0) / 1000
        self.index_ = 0
        self.done_ = False
        self.then_ = datetime.now()

    def __iter__(self):
        for event in self.events_:
            time.sleep(self.sleep_time_)
            if self.done_:
                return
            logging.debug("yielding %d" % event)
            yield BogusProto(event)

    def done(self):
        return self.done_

    def cancel(self):
        logging.debug("cancel")
        self.done_ = True
        return None


class CertSelfTest(BaseTestClass):

    def setup_test(self):
        return True

    def teardown_test(self):
        return True

    def test_assert_none_passes(self):
        with EventStream(FetchEvents(events=[], delay_ms=50)) as event_stream:
            event_stream.assert_none(timeout=timedelta(milliseconds=10))

    def test_assert_none_passes_after_one_second(self):
        with EventStream(FetchEvents([1], delay_ms=1500)) as event_stream:
            event_stream.assert_none(timeout=timedelta(seconds=1.0))

    def test_assert_none_fails(self):
        try:
            with EventStream(FetchEvents(events=[17],
                                         delay_ms=50)) as event_stream:
                event_stream.assert_none(timeout=timedelta(seconds=1))
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assert_none_matching_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            event_stream.assert_none_matching(
                lambda data: data.value_ == 4, timeout=timedelta(seconds=0.15))

    def test_assert_none_matching_passes_after_1_second(self):
        with EventStream(FetchEvents(events=[1, 2, 3, 4],
                                     delay_ms=400)) as event_stream:
            event_stream.assert_none_matching(
                lambda data: data.value_ == 4, timeout=timedelta(seconds=1))

    def test_assert_none_matching_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                event_stream.assert_none_matching(
                    lambda data: data.value_ == 2, timeout=timedelta(seconds=1))
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assert_occurs_at_least_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3, 1, 2, 3],
                                     delay_ms=40)) as event_stream:
            event_stream.assert_event_occurs(
                lambda data: data.value_ == 1,
                timeout=timedelta(milliseconds=300),
                at_least_times=2)

    def test_assert_occurs_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            event_stream.assert_event_occurs(
                lambda data: data.value_ == 1, timeout=timedelta(seconds=1))

    def test_assert_occurs_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                event_stream.assert_event_occurs(
                    lambda data: data.value_ == 4, timeout=timedelta(seconds=1))
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assert_occurs_at_most_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3, 4],
                                     delay_ms=50)) as event_stream:
            event_stream.assert_event_occurs_at_most(
                lambda data: data.value_ < 4,
                timeout=timedelta(seconds=1),
                at_most_times=3)

    def test_assert_occurs_at_most_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3, 4],
                                         delay_ms=50)) as event_stream:
                event_stream.assert_event_occurs_at_most(
                    lambda data: data.value_ > 1,
                    timeout=timedelta(seconds=1),
                    at_most_times=2)
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_skip_a_test(self):
        asserts.skip("Skipping this test because it's blocked by b/xyz")
        assert False

    def test_nested_packets(self):
        handle = 123
        inside = hci_packets.ReadScanEnableBuilder()
        logging.debug(inside.Serialize())
        logging.debug("building outside")
        outside = hci_packets.AclPacketBuilder(
            handle,
            hci_packets.PacketBoundaryFlag.FIRST_NON_AUTOMATICALLY_FLUSHABLE,
            hci_packets.BroadcastFlag.POINT_TO_POINT, inside)
        logging.debug(outside.Serialize())
        logging.debug("Done!")

    def test_l2cap_config_options(self):
        mtu_opt = l2cap_packets.MtuConfigurationOption()
        mtu_opt.mtu = 123
        fcs_opt = l2cap_packets.FrameCheckSequenceOption()
        fcs_opt.fcs_type = l2cap_packets.FcsType.DEFAULT
        request = l2cap_packets.ConfigurationRequestBuilder(
            0x1d,  # Command ID
            0xc1d,  # Channel ID
            l2cap_packets.Continuation.END,
            [mtu_opt, fcs_opt])
        request_b_frame = l2cap_packets.BasicFrameBuilder(0x01, request)
        handle = 123
        wrapped = hci_packets.AclPacketBuilder(
            handle,
            hci_packets.PacketBoundaryFlag.FIRST_NON_AUTOMATICALLY_FLUSHABLE,
            hci_packets.BroadcastFlag.POINT_TO_POINT, request_b_frame)
        # Size is ACL (4) + L2CAP (4) + Configure (8) + MTU (4) + FCS (3)
        asserts.assert_true(
            len(wrapped.Serialize()) == 23, "Packet serialized incorrectly")

    def test_assertThat_boolean_success(self):
        assertThat(True).isTrue()
        assertThat(False).isFalse()

    def test_assertThat_boolean_falseIsTrue(self):
        try:
            assertThat(False).isTrue()
        except Exception as e:
            return True
        return False

    def test_assertThat_boolean_trueIsFalse(self):
        try:
            assertThat(True).isFalse()
        except Exception as e:
            return True
        return False

    def test_assertThat_object_success(self):
        assertThat("this").isEqualTo("this")
        assertThat("this").isNotEqualTo("that")
        assertThat(None).isNone()
        assertThat("this").isNotNone()

    def test_assertThat_object_isEqualToFails(self):
        try:
            assertThat("this").isEqualTo("that")
        except Exception as e:
            return True
        return False

    def test_assertThat_object_isNotEqualToFails(self):
        try:
            assertThat("this").isNotEqualTo("this")
        except Exception as e:
            return True
        return False

    def test_assertThat_object_isNoneFails(self):
        try:
            assertThat("this").isNone()
        except Exception as e:
            return True
        return False

    def test_assertThat_object_isNotNoneFails(self):
        try:
            assertThat(None).isNotNone()
        except Exception as e:
            return True
        return False

    def test_assertThat_eventStream_emits_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            assertThat(event_stream).emits(lambda data: data.value_ == 1)

    def test_assertThat_eventStream_emits_then_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            assertThat(event_stream).emits(lambda data: data.value_ == 1).then(
                lambda data: data.value_ == 3)

    def test_assertThat_eventStream_emits_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emits(lambda data: data.value_ == 4)
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assertThat_eventStream_emits_then_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emits(
                    lambda data: data.value_ == 1).emits(
                        lambda data: data.value_ == 4)
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assertThat_eventStream_emitsInOrder_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            assertThat(event_stream).emits(
                lambda data: data.value_ == 1,
                lambda data: data.value_ == 2).inOrder()

    def test_assertThat_eventStream_emitsInAnyOrder_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            assertThat(event_stream).emits(
                lambda data: data.value_ == 2,
                lambda data: data.value_ == 1).inAnyOrder().then(
                    lambda data: data.value_ == 3)

    def test_assertThat_eventStream_emitsInOrder_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emits(
                    lambda data: data.value_ == 2,
                    lambda data: data.value_ == 1).inOrder()
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assertThat_eventStream_emitsInAnyOrder_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emits(
                    lambda data: data.value_ == 4,
                    lambda data: data.value_ == 1).inAnyOrder()
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assertThat_emitsNone_passes(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            assertThat(event_stream).emitsNone(
                lambda data: data.value_ == 4,
                timeout=timedelta(seconds=0.15)).thenNone(
                    lambda data: data.value_ == 5,
                    timeout=timedelta(seconds=0.15))

    def test_assertThat_emitsNone_passes_after_1_second(self):
        with EventStream(FetchEvents(events=[1, 2, 3, 4],
                                     delay_ms=400)) as event_stream:
            assertThat(event_stream).emitsNone(
                lambda data: data.value_ == 4, timeout=timedelta(seconds=1))

    def test_assertThat_emitsNone_fails(self):
        try:
            with EventStream(FetchEvents(events=[1, 2, 3],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emitsNone(
                    lambda data: data.value_ == 2, timeout=timedelta(seconds=1))
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_assertThat_emitsNone_zero_passes(self):
        with EventStream(FetchEvents(events=[], delay_ms=50)) as event_stream:
            assertThat(event_stream).emitsNone(
                timeout=timedelta(milliseconds=10)).thenNone(
                    timeout=timedelta(milliseconds=10))

    def test_assertThat_emitsNone_zero_passes_after_one_second(self):
        with EventStream(FetchEvents([1], delay_ms=1500)) as event_stream:
            assertThat(event_stream).emitsNone(timeout=timedelta(seconds=1.0))

    def test_assertThat_emitsNone_zero_fails(self):
        try:
            with EventStream(FetchEvents(events=[17],
                                         delay_ms=50)) as event_stream:
                assertThat(event_stream).emitsNone(timeout=timedelta(seconds=1))
        except Exception as e:
            logging.debug(e)
            return True  # Failed as expected
        return False

    def test_filtering_event_stream_none_filter_function(self):
        with EventStream(FetchEvents(events=[1, 2, 3],
                                     delay_ms=50)) as event_stream:
            filtered_event_stream = FilteringEventStream(event_stream, None)
            assertThat(filtered_event_stream)\
                .emits(lambda data: data.value_ == 1)\
                .then(lambda data: data.value_ == 3)

    def test_metadata_empty(self):
        my_content = [{}]

        class TestClass:

            def record_data(self, content):
                my_content[0] = content

            @metadata()
            def sample_pass_test(self):
                pass

            @metadata()
            def sample_skipped_test(self):
                asserts.skip("SKIP")

            @metadata()
            def sample_failed_test(self):
                asserts.fail("FAIL")

        test_class = TestClass()

        try:
            test_class.sample_pass_test()
        except Exception:
            asserts.fail("Should not raise exception")
        finally:
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_NAME)],
                                 "sample_pass_test")
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_CLASS)],
                                 "TestClass")

        try:
            test_class.sample_skipped_test()
        except Exception:
            pass
        finally:
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_NAME)],
                                 "sample_skipped_test")
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_CLASS)],
                                 "TestClass")

        try:
            test_class.sample_failed_test()
        except Exception:
            pass
        finally:
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_NAME)],
                                 "sample_failed_test")
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_CLASS)],
                                 "TestClass")

    def test_metadata_empty_no_function_call(self):
        my_content = [{}]

        class TestClass:

            def record_data(self, content):
                my_content[0] = content

            @metadata()
            def sample_pass_test(self):
                pass

        test_class = TestClass()

        try:
            test_class.sample_pass_test()
        except Exception:
            asserts.fail("Should not raise exception")
        finally:
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_NAME)],
                                 "sample_pass_test")
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_CLASS)],
                                 "TestClass")

    def test_metadata_pts_test_id(self):
        my_content = [{}]

        class TestClass:

            def record_data(self, content):
                my_content[0] = content

            @metadata(pts_test_id="Hello World")
            def sample_pass_test(self):
                pass

        test_class = TestClass()

        try:
            test_class.sample_pass_test()
        except Exception:
            asserts.fail("Should not raise exception")
        finally:
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_NAME)],
                                 "sample_pass_test")
            asserts.assert_equal(my_content[0][str(MetadataKey.TEST_CLASS)],
                                 "TestClass")
            asserts.assert_equal(my_content[0][str(MetadataKey.PTS_TEST_ID)],
                                 "Hello World")
