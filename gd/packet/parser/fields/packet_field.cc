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

#include "fields/packet_field.h"

PacketField::PacketField(std::string name, ParseLocation loc) : loc_(loc), name_(name) {}

std::string PacketField::GetDebugName() const {
  return "Field{Type:" + GetFieldType() + ", Name:" + GetName() + "}";
}

ParseLocation PacketField::GetLocation() const {
  return loc_;
}

std::string PacketField::GetName() const {
  return name_;
}

Size PacketField::GetBuilderSize() const {
  return GetSize();
}

void PacketField::GenBounds(std::ostream& s, Size start_offset, Size end_offset, Size field_size) const {
  // In order to find field_begin and field_end, we must have two of the three Sizes.
  if ((start_offset.empty() && field_size.empty()) || (start_offset.empty() && end_offset.empty()) ||
      (end_offset.empty() && field_size.empty())) {
    ERROR(this) << "GenBounds called without enough information. " << start_offset << end_offset << field_size;
  }

  if (start_offset.bits() % 8 != 0 || end_offset.bits() % 8 != 0) {
    ERROR(this) << "Can not find the bounds of a field at a non byte-aligned offset." << start_offset << end_offset;
  }

  if (!start_offset.empty()) {
    s << "size_t field_begin = (" << start_offset << ") / 8;";
  } else {
    s << "size_t field_begin = end_index - (" << end_offset << " + " << field_size << ") / 8;";
  }

  if (!end_offset.empty()) {
    s << "size_t field_end = end_index - (" << end_offset << ") / 8;";
    // If the field has a known size, use the minimum for the end
    if (!field_size.empty()) {
      s << "size_t field_sized_end = field_begin + (" << field_size << ") / 8;";
      s << "if (field_sized_end < field_end) { field_end = field_sized_end; }";
    }
  } else {
    s << "size_t field_end = field_begin + (" << field_size << ") / 8;";
  }
}

bool PacketField::GenBuilderMember(std::ostream& s) const {
  return GenBuilderParameter(s);
}
