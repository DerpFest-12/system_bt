/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "l2cap/l2cap_packets.h"

#include <gtest/gtest.h>
#include <forward_list>
#include <memory>

#include "os/log.h"
#include "packet/bit_inserter.h"
#include "packet/raw_builder.h"

using bluetooth::packet::BitInserter;
using bluetooth::packet::RawBuilder;
using std::vector;

namespace {
vector<uint8_t> extended_information_start_frame = {
    0x0B /* First size byte */,
    0x00 /* Second size byte */,
    0xc1 /* First ChannelId byte */,
    0xc2,
    0x4A /* 0x12 ReqSeq, Final, IFrame */,
    0xD0 /* 0x13 ReqSeq */,
    0x89 /* 0x21 TxSeq sar = START */,
    0x8C /* 0x23 TxSeq  */,
    0x10 /* first length byte */,
    0x11,
    0x01 /* first payload byte */,
    0x02,
    0x03,
    0x04,
    0x05,
};
}  // namespace

namespace bluetooth {
namespace l2cap {

TEST(L2capPacketTest, extendedInformationStartFrameTest) {
  uint16_t channel_id = 0xc2c1;
  uint16_t l2cap_sdu_length = 0x1110;
  Final f = Final::POLL_RESPONSE;
  uint16_t req_seq = 0x1312;
  uint16_t tx_seq = 0x2321;

  std::unique_ptr<RawBuilder> payload = std::make_unique<RawBuilder>();
  payload->AddOctets4(0x04030201);
  payload->AddOctets1(0x05);

  auto packet = ExtendedInformationStartFrameBuilder::Create(channel_id, f, req_seq, tx_seq, l2cap_sdu_length,
                                                             std::move(payload));

  ASSERT_EQ(extended_information_start_frame.size(), packet->size());
  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);
  PacketView<true> packet_bytes_view(packet_bytes);
  ASSERT_EQ(extended_information_start_frame.size(), packet_bytes_view.size());

  BasicFrameView basic_frame_view = BasicFrameView::Create(packet_bytes_view);
  ASSERT_TRUE(basic_frame_view.IsValid());
  ASSERT_EQ(channel_id, basic_frame_view.GetChannelId());

  StandardFrameView standard_frame_view = StandardFrameView::Create(basic_frame_view);
  ASSERT_TRUE(standard_frame_view.IsValid());
  ASSERT_EQ(FrameType::I_FRAME, standard_frame_view.GetFrameType());
}

vector<uint8_t> i_frame_with_fcs = {
    0x0E, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x38, 0x61,
};
TEST(L2capPacketTest, iFrameWithFcsTest) {
  uint16_t channel_id = 0x0040;
  SegmentationAndReassembly sar = SegmentationAndReassembly::UNSEGMENTED;  // 0
  uint16_t req_seq = 0;
  uint16_t tx_seq = 1;
  RetransmissionDisable r = RetransmissionDisable::NORMAL;  // 0

  std::unique_ptr<RawBuilder> payload = std::make_unique<RawBuilder>();
  vector<uint8_t> payload_bytes = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
  payload->AddOctets(payload_bytes);

  auto packet = StandardInformationFrameWithFcsBuilder::Create(channel_id, tx_seq, r, req_seq, sar, std::move(payload));

  ASSERT_EQ(i_frame_with_fcs.size(), packet->size());
  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);
  PacketView<true> packet_bytes_view(packet_bytes);
  ASSERT_EQ(i_frame_with_fcs.size(), packet_bytes_view.size());

  for (size_t i = 0; i < i_frame_with_fcs.size(); i++) {
    ASSERT_EQ(i_frame_with_fcs[i], packet_bytes_view[i]);
  }

  BasicFrameWithFcsView basic_frame_view = BasicFrameWithFcsView::Create(packet_bytes_view);
  ASSERT_TRUE(basic_frame_view.IsValid());
  ASSERT_EQ(channel_id, basic_frame_view.GetChannelId());

  StandardFrameWithFcsView standard_frame_view = StandardFrameWithFcsView::Create(basic_frame_view);
  ASSERT_TRUE(standard_frame_view.IsValid());
  ASSERT_EQ(FrameType::I_FRAME, standard_frame_view.GetFrameType());

  StandardInformationFrameWithFcsView information_frame_view =
      StandardInformationFrameWithFcsView::Create(standard_frame_view);
  ASSERT_TRUE(information_frame_view.IsValid());
  ASSERT_EQ(sar, information_frame_view.GetSar());
  ASSERT_EQ(req_seq, information_frame_view.GetReqSeq());
  ASSERT_EQ(tx_seq, information_frame_view.GetTxSeq());
  ASSERT_EQ(r, information_frame_view.GetR());
}

vector<uint8_t> rr_frame_with_fcs = {
    0x04, 0x00, 0x40, 0x00, 0x01, 0x01, 0xD4, 0x14,
};
TEST(L2capPacketTest, rrFrameWithFcsTest) {
  uint16_t channel_id = 0x0040;
  SupervisoryFunction s = SupervisoryFunction::RECEIVER_READY;  // 0
  RetransmissionDisable r = RetransmissionDisable::NORMAL;      // 0
  uint16_t req_seq = 1;

  auto packet = StandardSupervisoryFrameWithFcsBuilder::Create(channel_id, s, r, req_seq);

  ASSERT_EQ(rr_frame_with_fcs.size(), packet->size());
  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);
  PacketView<true> packet_bytes_view(packet_bytes);
  ASSERT_EQ(rr_frame_with_fcs.size(), packet_bytes_view.size());

  for (size_t i = 0; i < rr_frame_with_fcs.size(); i++) {
    ASSERT_EQ(rr_frame_with_fcs[i], packet_bytes_view[i]);
  }

  BasicFrameWithFcsView basic_frame_view = BasicFrameWithFcsView::Create(packet_bytes_view);
  ASSERT_TRUE(basic_frame_view.IsValid());
  ASSERT_EQ(channel_id, basic_frame_view.GetChannelId());

  StandardFrameWithFcsView standard_frame_view = StandardFrameWithFcsView::Create(basic_frame_view);
  ASSERT_TRUE(standard_frame_view.IsValid());
  ASSERT_EQ(FrameType::S_FRAME, standard_frame_view.GetFrameType());

  StandardSupervisoryFrameWithFcsView supervisory_frame_view =
      StandardSupervisoryFrameWithFcsView::Create(standard_frame_view);
  ASSERT_TRUE(supervisory_frame_view.IsValid());
  ASSERT_EQ(s, supervisory_frame_view.GetS());
  ASSERT_EQ(r, supervisory_frame_view.GetR());
  ASSERT_EQ(req_seq, supervisory_frame_view.GetReqSeq());
}
}  // namespace l2cap
}  // namespace bluetooth
