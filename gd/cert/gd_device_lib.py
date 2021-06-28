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

from abc import ABC
from datetime import datetime
import inspect
import logging
import os
import pathlib
import shutil
import signal
import socket
import subprocess
import time
from typing import List

import grpc

from google.protobuf import empty_pb2 as empty_proto

from cert.async_subprocess_logger import AsyncSubprocessLogger
from cert.logging_client_interceptor import LoggingClientInterceptor
from cert.os_utils import get_gd_root
from cert.os_utils import read_crash_snippet_and_log_tail
from cert.os_utils import is_subprocess_alive
from cert.os_utils import make_ports_available
from cert.os_utils import TerminalColor
from facade import rootservice_pb2_grpc as facade_rootservice_pb2_grpc
from hal import hal_facade_pb2_grpc
from hci.facade import hci_facade_pb2_grpc
from hci.facade import acl_manager_facade_pb2_grpc
from hci.facade import controller_facade_pb2_grpc
from hci.facade import le_acl_manager_facade_pb2_grpc
from hci.facade import le_advertising_manager_facade_pb2_grpc
from hci.facade import le_initiator_address_facade_pb2_grpc
from hci.facade import le_scanning_manager_facade_pb2_grpc
from l2cap.classic import facade_pb2_grpc as l2cap_facade_pb2_grpc
from l2cap.le import facade_pb2_grpc as l2cap_le_facade_pb2_grpc
from iso import facade_pb2_grpc as iso_facade_pb2_grpc
from neighbor.facade import facade_pb2_grpc as neighbor_facade_pb2_grpc
from security import facade_pb2_grpc as security_facade_pb2_grpc
from shim.facade import facade_pb2_grpc as shim_facade_pb2_grpc

MOBLY_CONTROLLER_CONFIG_NAME = "GdDevice"
ACTS_CONTROLLER_REFERENCE_NAME = "gd_devices"


def create_core(configs):
    if not configs:
        raise Exception("Configuration is empty")
    elif not isinstance(configs, list):
        raise Exception("Configuration should be a list")


def destroy_core(devices):
    for device in devices:
        try:
            device.teardown()
        except:
            logging.exception("[%s] Failed to clean up properly due to" % device.label)


def get_info(devices):
    return []


def replace_vars(string, config):
    serial_number = config.get("serial_number")
    if serial_number is None:
        serial_number = ""
    rootcanal_port = config.get("rootcanal_port")
    if rootcanal_port is None:
        rootcanal_port = ""
    if serial_number == "DUT" or serial_number == "CERT":
        raise Exception("Did you forget to configure the serial number?")
    return string.replace("$GD_ROOT", get_gd_root()) \
                 .replace("$(grpc_port)", config.get("grpc_port")) \
                 .replace("$(grpc_root_server_port)", config.get("grpc_root_server_port")) \
                 .replace("$(rootcanal_port)", rootcanal_port) \
                 .replace("$(signal_port)", config.get("signal_port")) \
                 .replace("$(serial_number)", serial_number)


class GdDeviceBaseCore(ABC):
    """
    Base class of GdDeviceBase that covers common traits unbound of ACTS dependency
    """

    WAIT_CHANNEL_READY_TIMEOUT_SECONDS = 10

    def __init__(self, grpc_port, grpc_root_server_port, signal_port, cmd, label, type_identifier, name, verbose_mode,
                 log_path_base, test_runner_base_path):
        """Base GD device, common traits for both device based and host only GD
        cert tests
        :param grpc_port: main gRPC service port
        :param grpc_root_server_port: gRPC root server port
        :param signal_port: signaling port for backing process start up
        :param cmd: list of arguments to run in backing process
        :param label: device label used in logs
        :param type_identifier: device type identifier used in logs
        :param name: name of device used in logs
        :param log_path_base: path to test case logs
        :param test_runner_base_path: path to test run logs
        """
        self.verbose_mode = verbose_mode
        self.grpc_root_server_port = int(grpc_root_server_port)
        self.grpc_port = int(grpc_port)
        self.signal_port = int(signal_port)
        self.name = name
        self.type_identifier = type_identifier
        self.label = label
        self.log_path_base = log_path_base
        self.test_runner_base_path = test_runner_base_path
        self.backing_process_log_path = os.path.join(self.log_path_base,
                                                     '%s_%s_backing_logs.txt' % (self.type_identifier, self.label))
        if "--btsnoop=" not in " ".join(cmd):
            cmd.append("--btsnoop=%s" % os.path.join(self.log_path_base, '%s_btsnoop_hci.log' % self.label))
        if "--btsnooz=" not in " ".join(cmd):
            cmd.append("--btsnooz=%s" % os.path.join(self.log_path_base, '%s_btsnooz_hci.log' % self.label))
        if "--btconfig=" not in " ".join(cmd):
            cmd.append("--btconfig=%s" % os.path.join(self.log_path_base, '%s_bt_config.conf' % self.label))
        self.cmd = cmd
        self.environment = os.environ.copy()
        if "cert" in self.label:
            self.terminal_color = TerminalColor.BLUE
        else:
            self.terminal_color = TerminalColor.YELLOW

    def setup(self):
        """Core method to set up device for test
        :return:
        """
        self.signal_port_available = make_ports_available([self.signal_port])
        if self.signal_port_available is not True:
            return
        # Start backing process
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as signal_socket:
            # Setup signaling socket
            signal_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            signal_socket.bind(("localhost", self.signal_port))
            signal_socket.listen(1)
            signal_socket.settimeout(300)  # 5 minute timeout for blocking socket operations

            # Start backing process
            logging.debug("Running %s" % " ".join(self.cmd))
            self.backing_process = subprocess.Popen(
                self.cmd,
                cwd=get_gd_root(),
                env=self.environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True)
            if not self.backing_process:
                return
            self.is_backing_process_alive = is_subprocess_alive(self.backing_process)
            if not self.is_backing_process_alive:
                return

            # Wait for process to be ready
            logging.debug("Waiting for backing_process accept.")
            signal_socket.accept()

        self.backing_process_logger = AsyncSubprocessLogger(
            self.backing_process, [self.backing_process_log_path],
            log_to_stdout=self.verbose_mode,
            tag=self.label,
            color=self.terminal_color)

        # Setup gRPC management channels
        self.grpc_root_server_channel = grpc.insecure_channel("localhost:%d" % self.grpc_root_server_port)
        self.grpc_channel = grpc.insecure_channel("localhost:%d" % self.grpc_port)

        if self.verbose_mode:
            self.grpc_channel = grpc.intercept_channel(self.grpc_channel, LoggingClientInterceptor(self.label))

        # Establish services from facades
        self.rootservice = facade_rootservice_pb2_grpc.RootFacadeStub(self.grpc_root_server_channel)
        self.hal = hal_facade_pb2_grpc.HciHalFacadeStub(self.grpc_channel)
        self.controller_read_only_property = facade_rootservice_pb2_grpc.ReadOnlyPropertyStub(self.grpc_channel)
        self.hci = hci_facade_pb2_grpc.HciFacadeStub(self.grpc_channel)
        self.l2cap = l2cap_facade_pb2_grpc.L2capClassicModuleFacadeStub(self.grpc_channel)
        self.l2cap_le = l2cap_le_facade_pb2_grpc.L2capLeModuleFacadeStub(self.grpc_channel)
        self.iso = iso_facade_pb2_grpc.IsoModuleFacadeStub(self.grpc_channel)
        self.hci_acl_manager = acl_manager_facade_pb2_grpc.AclManagerFacadeStub(self.grpc_channel)
        self.hci_le_acl_manager = le_acl_manager_facade_pb2_grpc.LeAclManagerFacadeStub(self.grpc_channel)
        self.hci_le_initiator_address = le_initiator_address_facade_pb2_grpc.LeInitiatorAddressFacadeStub(
            self.grpc_channel)
        self.hci_controller = controller_facade_pb2_grpc.ControllerFacadeStub(self.grpc_channel)
        self.hci_controller.GetMacAddressSimple = lambda: self.hci_controller.GetMacAddress(empty_proto.Empty()).address
        self.hci_controller.GetLocalNameSimple = lambda: self.hci_controller.GetLocalName(empty_proto.Empty()).name
        self.hci_le_advertising_manager = le_advertising_manager_facade_pb2_grpc.LeAdvertisingManagerFacadeStub(
            self.grpc_channel)
        self.hci_le_scanning_manager = le_scanning_manager_facade_pb2_grpc.LeScanningManagerFacadeStub(
            self.grpc_channel)
        self.neighbor = neighbor_facade_pb2_grpc.NeighborFacadeStub(self.grpc_channel)
        self.security = security_facade_pb2_grpc.SecurityModuleFacadeStub(self.grpc_channel)
        self.shim = shim_facade_pb2_grpc.ShimFacadeStub(self.grpc_channel)

    def get_crash_snippet_and_log_tail(self):
        if is_subprocess_alive(self.backing_process):
            return None, None

        return read_crash_snippet_and_log_tail(self.backing_process_log_path)

    def teardown(self):
        """Core method to tear down device and clean up any resources
        :return:
        """
        self.grpc_channel.close()
        self.grpc_root_server_channel.close()
        stop_signal = signal.SIGINT
        self.backing_process.send_signal(stop_signal)
        try:
            return_code = self.backing_process.wait(timeout=self.WAIT_CHANNEL_READY_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            logging.error("[%s] Failed to interrupt backing process via SIGINT, sending SIGKILL" % self.label)
            stop_signal = signal.SIGKILL
            self.backing_process.kill()
            try:
                return_code = self.backing_process.wait(timeout=self.WAIT_CHANNEL_READY_TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired:
                logging.error("Failed to kill backing process")
                return_code = -65536
        if return_code not in [-stop_signal, 0]:
            logging.error("backing process %s stopped with code: %d" % (self.label, return_code))
        self.backing_process_logger.stop()

    def wait_channel_ready(self):
        future = grpc.channel_ready_future(self.grpc_channel)
        try:
            future.result(timeout=self.WAIT_CHANNEL_READY_TIMEOUT_SECONDS)
        except grpc.FutureTimeoutError:
            raise


class GdHostOnlyDeviceCore():
    """
    Provide core methods of GdHostOnlyDevice class unbound of ACTS dependency
    """

    def generate_coverage_report(self, backing_process_profraw_path, label, test_runner_base_path, type_identifier,
                                 cmd):
        self.backing_process_profraw_path = backing_process_profraw_path
        self.label = label
        self.test_runner_base_path = test_runner_base_path
        self.type_identifier = type_identifier
        self.cmd = cmd

        if not self.backing_process_profraw_path.is_file():
            logging.info("[%s] Skip coverage report as there is no profraw file at %s" %
                         (self.label, str(self.backing_process_profraw_path)))
            return
        try:
            if self.backing_process_profraw_path.stat().st_size <= 0:
                logging.info("[%s] Skip coverage report as profraw file is empty at %s" %
                             (self.label, str(self.backing_process_profraw_path)))
                return
        except OSError:
            logging.info("[%s] Skip coverage report as profraw file is inaccessible at %s" %
                         (self.label, str(self.backing_process_profraw_path)))
            return
        llvm_binutils = pathlib.Path(get_gd_root()).joinpath("llvm_binutils").joinpath("bin")
        llvm_profdata = llvm_binutils.joinpath("llvm-profdata")
        if not llvm_profdata.is_file():
            logging.info(
                "[%s] Skip coverage report as llvm-profdata is not found at %s" % (self.label, str(llvm_profdata)))
            return
        llvm_cov = llvm_binutils.joinpath("llvm-cov")
        if not llvm_cov.is_file():
            logging.info("[%s] Skip coverage report as llvm-cov is not found at %s" % (self.label, str(llvm_cov)))
            return
        logging.info("[%s] Generating coverage report" % self.label)
        profdata_path = pathlib.Path(self.test_runner_base_path).joinpath(
            "%s_%s_backing_process_coverage.profdata" % (self.type_identifier, self.label))
        profdata_path_tmp = pathlib.Path(self.test_runner_base_path).joinpath(
            "%s_%s_backing_process_coverage_tmp.profdata" % (self.type_identifier, self.label))
        # Merge with existing profdata if possible
        profdata_cmd = [str(llvm_profdata), "merge", "-sparse", str(self.backing_process_profraw_path)]
        if profdata_path.is_file():
            profdata_cmd.append(str(profdata_path))
        profdata_cmd += ["-o", str(profdata_path_tmp)]
        result = subprocess.run(profdata_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            logging.warning("[%s] Failed to index profdata, cmd result: %r" % (self.label, result))
            profdata_path.unlink(missing_ok=True)
            return
        shutil.move(profdata_path_tmp, profdata_path)
        coverage_result_path = pathlib.Path(self.test_runner_base_path).joinpath(
            "%s_%s_backing_process_coverage.json" % (self.type_identifier, self.label))
        with coverage_result_path.open("w") as coverage_result_file:
            result = subprocess.run(
                [str(llvm_cov), "export", "--format=text", "--instr-profile", profdata_path, self.cmd[0]],
                stderr=subprocess.PIPE,
                stdout=coverage_result_file,
                cwd=os.path.join(get_gd_root()))
        if result.returncode != 0:
            logging.warning("[%s] Failed to generated coverage report, cmd result: %r" % (self.label, result))
            coverage_result_path.unlink(missing_ok=True)
            return
        coverage_summary_path = pathlib.Path(self.test_runner_base_path).joinpath(
            "%s_%s_backing_process_coverage_summary.txt" % (self.type_identifier, self.label))
        with coverage_summary_path.open("w") as coverage_summary_file:
            result = subprocess.run(
                [llvm_cov, "report", "--instr-profile", profdata_path, self.cmd[0]],
                stderr=subprocess.PIPE,
                stdout=coverage_summary_file,
                cwd=os.path.join(get_gd_root()))
        if result.returncode != 0:
            logging.warning("[%s] Failed to generated coverage summary, cmd result: %r" % (self.label, result))
            coverage_summary_path.unlink(missing_ok=True)
