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

from google.protobuf import empty_pb2 as empty_proto

from cert.gd_device_base import GdDeviceBase, replace_vars
from cert.event_stream import EventStream
from facade import rootservice_pb2_grpc as facade_rootservice_pb2_grpc
from hal import facade_pb2_grpc as hal_facade_pb2_grpc
from hci.facade import facade_pb2 as hci_facade
from hci.facade import facade_pb2_grpc as hci_facade_pb2_grpc
from hci.facade import acl_manager_facade_pb2_grpc
from hci.facade import controller_facade_pb2_grpc
from hci.facade import le_acl_manager_facade_pb2_grpc
from hci.facade import le_advertising_manager_facade_pb2_grpc
from hci.facade import le_scanning_manager_facade_pb2_grpc
from l2cap.classic import facade_pb2_grpc as l2cap_facade_pb2_grpc
from l2cap.le import facade_pb2_grpc as l2cap_le_facade_pb2_grpc
from neighbor.facade import facade_pb2_grpc as neighbor_facade_pb2_grpc
from security import facade_pb2_grpc as security_facade_pb2_grpc

ACTS_CONTROLLER_CONFIG_NAME = "GdDevice"
ACTS_CONTROLLER_REFERENCE_NAME = "gd_devices"


def create(configs):
    if not configs:
        raise Exception("Configuration is empty")
    elif not isinstance(configs, list):
        raise Exception("Configuration should be a list")
    return get_instances_with_configs(configs)


def destroy(devices):
    for device in devices:
        try:
            device.clean_up()
        except:
            device.log.exception("Failed to clean up properly.")


def get_info(devices):
    return []


def get_instances_with_configs(configs):
    print(configs)
    devices = []
    for config in configs:
        resolved_cmd = []
        for entry in config["cmd"]:
            logging.debug(entry)
            resolved_cmd.append(replace_vars(entry, config))
        devices.append(
            GdDevice(config["grpc_port"], config["grpc_root_server_port"],
                     config["signal_port"], resolved_cmd, config["label"],
                     config.get("serial_number", ""), config.get("name", "")))
    return devices


class GdDevice(GdDeviceBase):

    def __init__(self, grpc_port, grpc_root_server_port, signal_port, cmd,
                 label, serial_number, name):
        super().__init__(grpc_port, grpc_root_server_port, signal_port, cmd,
                         label, ACTS_CONTROLLER_CONFIG_NAME, serial_number,
                         name)

        # Facade stubs
        self.rootservice = facade_rootservice_pb2_grpc.RootFacadeStub(
            self.grpc_root_server_channel)
        self.hal = hal_facade_pb2_grpc.HciHalFacadeStub(self.grpc_channel)
        self.controller_read_only_property = facade_rootservice_pb2_grpc.ReadOnlyPropertyStub(
            self.grpc_channel)
        self.hci = hci_facade_pb2_grpc.HciLayerFacadeStub(self.grpc_channel)
        self.hci.register_for_events = self.__register_for_hci_events
        self.hci.new_event_stream = lambda: EventStream(self.hci.FetchEvents(empty_proto.Empty()))
        self.hci.send_command_with_complete = self.__send_hci_command_with_complete
        self.hci.send_command_with_status = self.__send_hci_command_with_status
        self.l2cap = l2cap_facade_pb2_grpc.L2capClassicModuleFacadeStub(
            self.grpc_channel)
        self.l2cap_le = l2cap_le_facade_pb2_grpc.L2capLeModuleFacadeStub(
            self.grpc_channel)
        self.hci_acl_manager = acl_manager_facade_pb2_grpc.AclManagerFacadeStub(
            self.grpc_channel)
        self.hci_le_acl_manager = le_acl_manager_facade_pb2_grpc.LeAclManagerFacadeStub(
            self.grpc_channel)
        self.hci_controller = controller_facade_pb2_grpc.ControllerFacadeStub(
            self.grpc_channel)
        self.hci_controller.GetMacAddressSimple = lambda : self.hci_controller.GetMacAddress(empty_proto.Empty()).address
        self.hci_controller.GetLocalNameSimple = lambda : self.hci_controller.GetLocalName(empty_proto.Empty()).name
        self.hci_le_advertising_manager = le_advertising_manager_facade_pb2_grpc.LeAdvertisingManagerFacadeStub(
            self.grpc_channel)
        self.hci_le_scanning_manager = le_scanning_manager_facade_pb2_grpc.LeScanningManagerFacadeStub(
            self.grpc_channel)
        self.neighbor = neighbor_facade_pb2_grpc.NeighborFacadeStub(
            self.grpc_channel)
        self.security = security_facade_pb2_grpc.SecurityModuleFacadeStub(
            self.grpc_channel)

    def __register_for_hci_events(self, *event_codes):
        for event_code in event_codes:
            msg = hci_facade.EventCodeMsg(code=int(event_code))
            self.hci.RegisterEventHandler(msg)

    def __send_hci_command_with_complete(self, command):
        cmd_bytes = bytes(command.Serialize())
        cmd = hci_facade.CommandMsg(command=cmd_bytes)
        self.hci.EnqueueCommandWithComplete(cmd)

    def __send_hci_command_with_status(self, command):
        cmd_bytes = bytes(command.Serialize())
        cmd = hci_facade.CommandMsg(command=cmd_bytes)
        self.hci.EnqueueCommandWithStatus(cmd)
