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

import logging
import os
from builtins import open
import signal
import socket
import subprocess
import time

from acts import error
from acts import tracelogger

import grpc

ANDROID_BUILD_TOP = os.environ.get('ANDROID_BUILD_TOP')
ANDROID_HOST_OUT = os.environ.get('ANDROID_HOST_OUT')

def replace_vars(string, config):
    return string.replace("$ANDROID_HOST_OUT", ANDROID_HOST_OUT) \
                 .replace("$(grpc_port)", config.get("grpc_port")) \
                 .replace("$(grpc_root_server_port)", config.get("grpc_root_server_port")) \
                 .replace("$(rootcanal_port)", config.get("rootcanal_port"))

class GdDeviceBase:
    def __init__(self, grpc_port, grpc_root_server_port, cmd, label, type_identifier):
        self.label = label if label is not None else grpc_port
        # logging.log_path only exists when this is used in an ACTS test run.
        log_path_base = getattr(logging, 'log_path', '/tmp/logs')
        self.log = tracelogger.TraceLogger(
            GdDeviceBaseLoggerAdapter(logging.getLogger(), {
                'device': label,
                'type_identifier' : type_identifier
            }))

        backing_process_logpath = os.path.join(
            log_path_base, '%s_%s_backing_logs.txt' % (type_identifier, label))
        self.backing_process_logs = open(backing_process_logpath, 'w')

        btsnoop_path = os.path.join(log_path_base, '%s_btsnoop_hci.log' % label)
        cmd.append("--btsnoop=" + btsnoop_path)

        tester_signal_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        socket_address = os.path.join(
            log_path_base, '%s_socket' % type_identifier)
        tester_signal_socket.bind(socket_address)
        tester_signal_socket.listen(1)

        cmd.append("--tester-signal-socket=" + socket_address)

        self.backing_process = subprocess.Popen(
            cmd,
            cwd=ANDROID_BUILD_TOP,
            env=os.environ.copy(),
            stdout=self.backing_process_logs,
            stderr=self.backing_process_logs)
        tester_signal_socket.accept()
        tester_signal_socket.close()

        self.grpc_root_server_channel = grpc.insecure_channel("localhost:" + grpc_root_server_port)
        self.grpc_port = int(grpc_port)
        self.grpc_channel = grpc.insecure_channel("localhost:" + grpc_port)

    def clean_up(self):
        self.grpc_channel.close()
        self.grpc_root_server_channel.close()
        self.backing_process.send_signal(signal.SIGINT)
        backing_process_return_code = self.backing_process.wait()
        self.backing_process_logs.close()
        if backing_process_return_code != 0:
            logging.error("backing process stopped with code: %d" %
                          backing_process_return_code)
            return False


class GdDeviceBaseLoggerAdapter(logging.LoggerAdapter):
    def process(self, msg, kwargs):
        msg = "[%s|%s] %s" % (self.extra["type_identifier"], self.extra["device"], msg)
        return (msg, kwargs)

class GdDeviceConfigError(Exception):
    """Raised when GdDevice configs are malformatted."""


class GdDeviceError(error.ActsError):
    """Raised when there is an error in GdDevice."""

