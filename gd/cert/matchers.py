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

import bluetooth_packets_python3 as bt_packets
from bluetooth_packets_python3 import l2cap_packets
from bluetooth_packets_python3.l2cap_packets import CommandCode, LeCommandCode
from bluetooth_packets_python3.l2cap_packets import ConnectionResponseResult
from bluetooth_packets_python3.l2cap_packets import InformationRequestInfoType
from bluetooth_packets_python3.l2cap_packets import LeCreditBasedConnectionResponseResult
import logging


class L2capMatchers(object):

    @staticmethod
    def ConnectionResponse(scid):
        return lambda packet: L2capMatchers._is_matching_connection_response(packet, scid)

    @staticmethod
    def ConnectionRequest():
        return lambda packet: L2capMatchers._is_control_frame_with_code(packet, CommandCode.CONNECTION_REQUEST)

    @staticmethod
    def ConfigurationResponse():
        return lambda packet: L2capMatchers._is_control_frame_with_code(packet, CommandCode.CONFIGURATION_RESPONSE)

    @staticmethod
    def ConfigurationRequest():
        return lambda packet: L2capMatchers._is_control_frame_with_code(packet, CommandCode.CONFIGURATION_REQUEST)

    @staticmethod
    def DisconnectionRequest():
        return lambda packet: L2capMatchers._is_control_frame_with_code(packet, CommandCode.DISCONNECTION_REQUEST)

    @staticmethod
    def DisconnectionResponse(scid, dcid):
        return lambda packet: L2capMatchers._is_matching_disconnection_response(packet, scid, dcid)

    @staticmethod
    def CommandReject():
        return lambda packet: L2capMatchers._is_control_frame_with_code(packet, CommandCode.COMMAND_REJECT)

    @staticmethod
    def LeCommandReject():
        return lambda packet: L2capMatchers._is_le_control_frame_with_code(packet, LeCommandCode.COMMAND_REJECT)

    @staticmethod
    def CreditBasedConnectionRequest(psm):
        return lambda packet: L2capMatchers._is_matching_credit_based_connection_request(packet, psm)

    @staticmethod
    def CreditBasedConnectionResponse(
            scid, result=LeCreditBasedConnectionResponseResult.SUCCESS):
        return lambda packet: L2capMatchers._is_matching_credit_based_connection_response(packet, scid, result)

    @staticmethod
    def LeDisconnectionRequest(scid, dcid):
        return lambda packet: L2capMatchers._is_matching_le_disconnection_request(packet, scid, dcid)

    @staticmethod
    def LeDisconnectionResponse(scid, dcid):
        return lambda packet: L2capMatchers._is_matching_le_disconnection_response(packet, scid, dcid)

    @staticmethod
    def SFrame(req_seq=None, f=None, s=None, p=None):
        return lambda packet: L2capMatchers._is_matching_supervisory_frame(packet, req_seq, f, s, p)

    @staticmethod
    def IFrame(tx_seq=None, payload=None, f=None):
        return lambda packet: L2capMatchers._is_matching_information_frame(packet, tx_seq, payload, f)

    @staticmethod
    def Data(payload):
        return lambda packet: packet.GetPayload().GetBytes() == payload

    @staticmethod
    def FirstLeIFrame(payload, sdu_size):
        return lambda packet: L2capMatchers._is_matching_first_le_i_frame(packet, payload, sdu_size)

    # this is a hack - should be removed
    @staticmethod
    def PartialData(payload):
        return lambda packet: payload in packet.GetPayload().GetBytes()

    # this is a hack - should be removed
    @staticmethod
    def PacketPayloadRawData(payload):
        return lambda packet: payload in packet.payload

    # this is a hack - should be removed
    @staticmethod
    def PacketPayloadWithMatchingPsm(psm):
        return lambda packet: None if psm != packet.psm else packet

    @staticmethod
    def ExtractBasicFrame(scid):
        return lambda packet: L2capMatchers._basic_frame_for(packet, scid)

    @staticmethod
    def InformationResponseExtendedFeatures(supports_ertm=None,
                                            supports_streaming=None,
                                            supports_fcs=None,
                                            supports_fixed_channels=None):
        return lambda packet: L2capMatchers._is_matching_information_response_extended_features(packet, supports_ertm, supports_streaming, supports_fcs, supports_fixed_channels)

    @staticmethod
    def _basic_frame(packet):
        if packet is None:
            return None
        return l2cap_packets.BasicFrameView(
            bt_packets.PacketViewLittleEndian(list(packet.payload)))

    @staticmethod
    def _basic_frame_for(packet, scid):
        frame = L2capMatchers._basic_frame(packet)
        if frame.GetChannelId() != scid:
            return None
        return frame

    @staticmethod
    def _information_frame(packet):
        standard_frame = l2cap_packets.StandardFrameView(packet)
        if standard_frame.GetFrameType() != l2cap_packets.FrameType.I_FRAME:
            return None
        return l2cap_packets.EnhancedInformationFrameView(standard_frame)

    @staticmethod
    def _supervisory_frame(packet):
        standard_frame = l2cap_packets.StandardFrameView(packet)
        if standard_frame.GetFrameType() != l2cap_packets.FrameType.S_FRAME:
            return None
        return l2cap_packets.EnhancedSupervisoryFrameView(standard_frame)

    @staticmethod
    def _is_matching_information_frame(packet, tx_seq, payload, f):
        frame = L2capMatchers._information_frame(packet)
        if frame is None:
            return False
        if tx_seq is not None and frame.GetTxSeq() != tx_seq:
            return False
        if payload is not None and frame.GetPayload().GetBytes() != payload:
            return False
        if f is not None and frame.GetF() != f:
            return False
        return True

    @staticmethod
    def _is_matching_supervisory_frame(packet, req_seq, f, s, p):
        frame = L2capMatchers._supervisory_frame(packet)
        if frame is None:
            return False
        if req_seq is not None and frame.GetReqSeq() != req_seq:
            return False
        if f is not None and frame.GetF() != f:
            return False
        if s is not None and frame.GetS() != s:
            return False
        if p is not None and frame.GetP() != p:
            return False
        return True

    @staticmethod
    def _is_matching_first_le_i_frame(packet, payload, sdu_size):
        first_le_i_frame = l2cap_packets.FirstLeInformationFrameView(packet)
        return first_le_i_frame.GetPayload().GetBytes(
        ) == payload and first_le_i_frame.GetL2capSduLength() == sdu_size

    @staticmethod
    def _control_frame(packet):
        if packet.GetChannelId() != 1:
            return None
        return l2cap_packets.ControlView(packet.GetPayload())

    @staticmethod
    def _le_control_frame(packet):
        if packet.GetChannelId() != 5:
            return None
        return l2cap_packets.LeControlView(packet.GetPayload())

    @staticmethod
    def control_frame_with_code(packet, code):
        frame = L2capMatchers._control_frame(packet)
        if frame is None or frame.GetCode() != code:
            return None
        return frame

    @staticmethod
    def le_control_frame_with_code(packet, code):
        frame = L2capMatchers._le_control_frame(packet)
        if frame is None or frame.GetCode() != code:
            return None
        return frame

    @staticmethod
    def _is_control_frame_with_code(packet, code):
        return L2capMatchers.control_frame_with_code(packet, code) is not None

    @staticmethod
    def _is_le_control_frame_with_code(packet, code):
        return L2capMatchers.le_control_frame_with_code(packet,
                                                        code) is not None

    @staticmethod
    def _is_matching_connection_response(packet, scid):
        frame = L2capMatchers.control_frame_with_code(
            packet, CommandCode.CONNECTION_RESPONSE)
        if frame is None:
            return False
        response = l2cap_packets.ConnectionResponseView(frame)
        return response.GetSourceCid() == scid and response.GetResult(
        ) == ConnectionResponseResult.SUCCESS and response.GetDestinationCid(
        ) != 0

    @staticmethod
    def _is_matching_disconnection_response(packet, scid, dcid):
        frame = L2capMatchers.control_frame_with_code(
            packet, CommandCode.DISCONNECTION_RESPONSE)
        if frame is None:
            return False
        response = l2cap_packets.DisconnectionResponseView(frame)
        return response.GetSourceCid() == scid and response.GetDestinationCid(
        ) == dcid

    @staticmethod
    def _is_matching_le_disconnection_response(packet, scid, dcid):
        frame = L2capMatchers.le_control_frame_with_code(
            packet, LeCommandCode.DISCONNECTION_RESPONSE)
        if frame is None:
            return False
        response = l2cap_packets.LeDisconnectionResponseView(frame)
        return response.GetSourceCid() == scid and response.GetDestinationCid(
        ) == dcid

    @staticmethod
    def _is_matching_le_disconnection_request(packet, scid, dcid):
        frame = L2capMatchers.le_control_frame_with_code(
            packet, LeCommandCode.DISCONNECTION_REQUEST)
        if frame is None:
            return False
        request = l2cap_packets.LeDisconnectionRequestView(frame)
        return request.GetSourceCid() == scid and request.GetDestinationCid(
        ) == dcid

    @staticmethod
    def _information_response_with_type(packet, info_type):
        frame = L2capMatchers.control_frame_with_code(
            packet, CommandCode.INFORMATION_RESPONSE)
        if frame is None:
            return None
        response = l2cap_packets.InformationResponseView(frame)
        if response.GetInfoType() != info_type:
            return None
        return response

    @staticmethod
    def _is_matching_information_response_extended_features(
            packet, supports_ertm, supports_streaming, supports_fcs,
            supports_fixed_channels):
        frame = L2capMatchers._information_response_with_type(
            packet, InformationRequestInfoType.EXTENDED_FEATURES_SUPPORTED)
        if frame is None:
            return False
        features = l2cap_packets.InformationResponseExtendedFeaturesView(frame)
        if supports_ertm is not None and features.GetEnhancedRetransmissionMode(
        ) != supports_ertm:
            return False
        if supports_streaming is not None and features.GetStreamingMode != supports_streaming:
            return False
        if supports_fcs is not None and features.GetFcsOption() != supports_fcs:
            return False
        if supports_fixed_channels is not None and features.GetFixedChannels(
        ) != supports_fixed_channels:
            return False
        return True

    @staticmethod
    def _is_matching_credit_based_connection_request(packet, psm):
        frame = L2capMatchers.le_control_frame_with_code(
            packet, LeCommandCode.LE_CREDIT_BASED_CONNECTION_REQUEST)
        if frame is None:
            return False
        request = l2cap_packets.LeCreditBasedConnectionRequestView(frame)
        return request.GetLePsm() == psm

    @staticmethod
    def _is_matching_credit_based_connection_response(packet, scid, result):
        frame = L2capMatchers.le_control_frame_with_code(
            packet, LeCommandCode.LE_CREDIT_BASED_CONNECTION_RESPONSE)
        if frame is None:
            return False
        response = l2cap_packets.LeCreditBasedConnectionResponseView(frame)
        return response.GetResult() == result and (
            result != LeCreditBasedConnectionResponseResult.SUCCESS or
            response.GetDestinationCid() != 0)
