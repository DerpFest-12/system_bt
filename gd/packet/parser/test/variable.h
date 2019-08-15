/******************************************************************************
 *
 *  Copyright 2019 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <stdint.h>
#include <optional>
#include <sstream>
#include <string>

#include "packet/bit_inserter.h"
#include "packet/iterator.h"

namespace bluetooth {
namespace packet {
namespace parser {
namespace test {

class Variable final {
 public:
  std::string data;

  Variable() = default;
  Variable(const Variable&) = default;
  Variable(const std::string& str);

  void Serialize(BitInserter& bi) const;

  size_t size() const;

  static Iterator<true> Parse(std::vector<Variable>& vec, Iterator<true> it);
  template <std::size_t arr_size>
  static Iterator<true> ParseArray(std::array<Variable, arr_size>& arr, std::size_t* arr_idx, Iterator<true> it);
};

template <std::size_t arr_size>
Iterator<true> Variable::ParseArray(std::array<Variable, arr_size>& arr, std::size_t* arr_idx, Iterator<true> it) {
  if (it.NumBytesRemaining() < 1) {
    return it;
  }
  size_t data_length = it.extract<uint8_t>();
  if (data_length > 255) {
    return it + it.NumBytesRemaining();
  }
  if (it.NumBytesRemaining() < data_length) {
    return it + it.NumBytesRemaining();
  }
  std::stringstream ss;
  for (size_t i = 0; i < data_length; i++) {
    ss << it.extract<char>();
  }
  arr[*arr_idx] = ss.str();
  return it;
}

}  // namespace test
}  // namespace parser
}  // namespace packet
}  // namespace bluetooth
