/*
 * Copyright 2020 The Android Open Source Project
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

#include "storage/config_cache.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>

namespace testing {

namespace {
std::string GetTestAddress(int i) {
  std::string res = "00:00:00:00:00:00";
  res.reserve(res.size() + 1);
  std::snprintf(res.data(), res.capacity(), "AA:BB:CC:DD:EE:%02d", i);
  return res;
}
}  // namespace

using bluetooth::storage::ConfigCache;

TEST(ConfigCacheTest, simple_set_get_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  auto value = config.GetProperty("A", "B");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "C");
}

TEST(ConfigCacheTest, empty_string_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "");
  auto value = config.GetProperty("A", "B");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "");
}

TEST(ConfigCacheTest, mac_address_set_get_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  auto value = config.GetProperty("A", "B");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "C");
  value = config.GetProperty("AA:BB:CC:DD:EE:FF", "B");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "C");
  EXPECT_FALSE(config.GetProperty("A", "BC"));
  EXPECT_FALSE(config.GetProperty("ABC", "B"));
}

TEST(ConfigCacheTest, has_section_and_property_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  EXPECT_TRUE(config.HasSection("A"));
  EXPECT_TRUE(config.HasSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.HasProperty("A", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  auto value = config.GetProperty("AA:BB:CC:DD:EE:FF", "C");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "D");
  value = config.GetProperty("AA:BB:CC:DD:EE:FF", "B");
  EXPECT_TRUE(value);
  EXPECT_EQ(*value, "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "E");
  value = config.GetProperty("AA:BB:CC:DD:EE:FF", "B");
  EXPECT_TRUE(value);
  EXPECT_THAT(value, Optional(StrEq("E")));
  EXPECT_FALSE(config.HasSection("Ab"));
  EXPECT_FALSE(config.HasSection("AA:11:CC:DD:EE:FF"));
  EXPECT_FALSE(config.HasProperty("A", "bB"));
  EXPECT_FALSE(config.HasProperty("AA:BB:11:DD:EE:FF", "B"));
}

TEST(ConfigCacheTest, remove_section_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  EXPECT_TRUE(config.HasSection("A"));
  EXPECT_TRUE(config.HasSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.HasProperty("A", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_TRUE(config.RemoveSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.RemoveSection("A"));
  EXPECT_FALSE(config.HasProperty("A", "B"));
  EXPECT_FALSE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
}

TEST(ConfigCacheTest, remove_property_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  EXPECT_TRUE(config.HasSection("A"));
  EXPECT_TRUE(config.HasSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.HasProperty("A", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "C"));
  EXPECT_TRUE(config.RemoveProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_FALSE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_FALSE(config.GetProperty("AA:BB:CC:DD:EE:FF", "B"));
}

TEST(ConfigCacheTest, remove_all_properties_from_section_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  EXPECT_TRUE(config.HasSection("A"));
  EXPECT_TRUE(config.HasSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.HasProperty("A", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_TRUE(config.HasProperty("AA:BB:CC:DD:EE:FF", "C"));
  EXPECT_TRUE(config.RemoveSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_FALSE(config.HasSection("AA:BB:CC:DD:EE:FF"));
  EXPECT_FALSE(config.HasProperty("AA:BB:CC:DD:EE:FF", "B"));
  EXPECT_FALSE(config.GetProperty("AA:BB:CC:DD:EE:FF", "C"));
}

TEST(ConfigCacheTest, get_persistent_devices_test) {
  ConfigCache config(100);
  config.SetProperty("A", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "B", "C");
  config.SetProperty("AA:BB:CC:DD:EE:FF", "C", "D");
  config.SetProperty("CC:DD:EE:FF:00:11", "LinkKey", "AABBAABBCCDDEE");
  EXPECT_TRUE(config.HasProperty("CC:DD:EE:FF:00:11", "LinkKey"));
  EXPECT_THAT(config.GetPersistentDevices(), ElementsAre("CC:DD:EE:FF:00:11"));
  config.SetProperty("AA:BB:CC:DD:EE:FF", "LinkKey", "DEERDEERDEER");
  EXPECT_THAT(config.GetPersistentDevices(), ElementsAre("CC:DD:EE:FF:00:11", "AA:BB:CC:DD:EE:FF"));
  EXPECT_TRUE(config.RemoveProperty("CC:DD:EE:FF:00:11", "LinkKey"));
  EXPECT_THAT(config.GetPersistentDevices(), ElementsAre("AA:BB:CC:DD:EE:FF"));
}

TEST(ConfigCacheTest, appoaching_temporary_cache_limit_test) {
  ConfigCache config(2);
  for (int i = 0; i < 10; ++i) {
    config.SetProperty(GetTestAddress(i), "Name", "Hello" + std::to_string(i));
    if (i % 2 == 0) {
      config.SetProperty(GetTestAddress(i), "LinkKey", "Key" + std::to_string(i));
    }
  }
  for (int i = 0; i < 10; ++i) {
    if (i % 2 == 0) {
      EXPECT_TRUE(config.HasSection(GetTestAddress(i)));
      EXPECT_TRUE(config.HasProperty(GetTestAddress(i), "LinkKey"));
      EXPECT_THAT(config.GetProperty(GetTestAddress(i), "Name"), Optional(StrEq("Hello" + std::to_string(i))));
    } else if (i >= 7) {
      EXPECT_TRUE(config.HasSection(GetTestAddress(i)));
      EXPECT_THAT(config.GetProperty(GetTestAddress(i), "Name"), Optional(StrEq("Hello" + std::to_string(i))));
    } else {
      EXPECT_FALSE(config.HasSection(GetTestAddress(i)));
    }
  }
  EXPECT_THAT(
      config.GetPersistentDevices(),
      ElementsAre(GetTestAddress(0), GetTestAddress(2), GetTestAddress(4), GetTestAddress(6), GetTestAddress(8)));
}

}  // namespace testing