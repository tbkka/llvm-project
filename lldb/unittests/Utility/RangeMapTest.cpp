//===-- RangeTest.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Utility/RangeMap.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace lldb_private;

using RangeDataVectorT = RangeDataVector<uint32_t, uint32_t, uint32_t>;
using EntryT = RangeDataVectorT::Entry;

static testing::Matcher<const EntryT *> EntryIs(uint32_t ID) {
  return testing::Pointee(testing::Field(&EntryT::data, ID));
}

std::vector<uint32_t> FindEntryIndexes(uint32_t address, RangeDataVectorT map) {
  std::vector<uint32_t> result;
  map.FindEntryIndexesThatContain(address, result);
  return result;
}

TEST(RangeDataVector, FindEntryThatContains) {
  RangeDataVectorT Map;
  uint32_t NextID = 0;
  Map.Append(EntryT(0, 10, NextID++));
  Map.Append(EntryT(10, 10, NextID++));
  Map.Append(EntryT(20, 10, NextID++));
  Map.Sort();

  EXPECT_THAT(Map.FindEntryThatContains(0), EntryIs(0));
  EXPECT_THAT(Map.FindEntryThatContains(9), EntryIs(0));
  EXPECT_THAT(Map.FindEntryThatContains(10), EntryIs(1));
  EXPECT_THAT(Map.FindEntryThatContains(19), EntryIs(1));
  EXPECT_THAT(Map.FindEntryThatContains(20), EntryIs(2));
  EXPECT_THAT(Map.FindEntryThatContains(29), EntryIs(2));
  EXPECT_THAT(Map.FindEntryThatContains(30), nullptr);
}

TEST(RangeDataVector, FindEntryThatContains_Overlap) {
  RangeDataVectorT Map;
  uint32_t NextID = 0;
  Map.Append(EntryT(0, 40, NextID++));
  Map.Append(EntryT(10, 20, NextID++));
  Map.Append(EntryT(20, 10, NextID++));
  Map.Sort();

  // With overlapping intervals, the intention seems to be to return the first
  // interval which contains the address.
  EXPECT_THAT(Map.FindEntryThatContains(25), EntryIs(0));

  // However, this does not always succeed.
  // TODO: This should probably return the range (0, 40) as well.
  EXPECT_THAT(Map.FindEntryThatContains(35), nullptr);
}

TEST(RangeDataVector, CustomSort) {
  // First the default ascending order sorting of the data field.
  auto Map = RangeDataVectorT();
  Map.Append(EntryT(0, 10, 50));
  Map.Append(EntryT(0, 10, 52));
  Map.Append(EntryT(0, 10, 53));
  Map.Append(EntryT(0, 10, 51));
  Map.Sort();

  EXPECT_THAT(Map.GetSize(), 4);
  EXPECT_THAT(Map.GetEntryRef(0).data, 50);
  EXPECT_THAT(Map.GetEntryRef(1).data, 51);
  EXPECT_THAT(Map.GetEntryRef(2).data, 52);
  EXPECT_THAT(Map.GetEntryRef(3).data, 53);

  // And then a custom descending order sorting of the data field.
  class CtorParam {};
  class CustomSort {
  public:
    CustomSort(CtorParam) {}
    bool operator()(const uint32_t a_data, const uint32_t b_data) {
      return a_data > b_data;
    }
  };
  using RangeDataVectorCustomSortT =
      RangeDataVector<uint32_t, uint32_t, uint32_t, 0, CustomSort>;
  using EntryT = RangeDataVectorT::Entry;

  auto MapC = RangeDataVectorCustomSortT(CtorParam());
  MapC.Append(EntryT(0, 10, 50));
  MapC.Append(EntryT(0, 10, 52));
  MapC.Append(EntryT(0, 10, 53));
  MapC.Append(EntryT(0, 10, 51));
  MapC.Sort();

  EXPECT_THAT(MapC.GetSize(), 4);
  EXPECT_THAT(MapC.GetEntryRef(0).data, 53);
  EXPECT_THAT(MapC.GetEntryRef(1).data, 52);
  EXPECT_THAT(MapC.GetEntryRef(2).data, 51);
  EXPECT_THAT(MapC.GetEntryRef(3).data, 50);
}

TEST(RangeDataVector, FindEntryIndexesThatContain) {
  RangeDataVectorT Map;
  Map.Append(EntryT(0, 10, 10));
  Map.Append(EntryT(10, 10, 11));
  Map.Append(EntryT(20, 10, 12));
  Map.Sort();

  EXPECT_THAT(FindEntryIndexes(0, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(9, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(10, Map), testing::ElementsAre(11));
  EXPECT_THAT(FindEntryIndexes(19, Map), testing::ElementsAre(11));
  EXPECT_THAT(FindEntryIndexes(20, Map), testing::ElementsAre(12));
  EXPECT_THAT(FindEntryIndexes(29, Map), testing::ElementsAre(12));
  EXPECT_THAT(FindEntryIndexes(30, Map), testing::ElementsAre());
}

TEST(RangeDataVector, FindEntryIndexesThatContain_Overlap) {
  RangeDataVectorT Map;
  Map.Append(EntryT(0, 40, 10));
  Map.Append(EntryT(10, 20, 11));
  Map.Append(EntryT(20, 10, 12));
  Map.Sort();

  EXPECT_THAT(FindEntryIndexes(0, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(9, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(10, Map), testing::ElementsAre(10, 11));
  EXPECT_THAT(FindEntryIndexes(19, Map), testing::ElementsAre(10, 11));
  EXPECT_THAT(FindEntryIndexes(20, Map), testing::ElementsAre(10, 11, 12));
  EXPECT_THAT(FindEntryIndexes(29, Map), testing::ElementsAre(10, 11, 12));
  EXPECT_THAT(FindEntryIndexes(30, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(39, Map), testing::ElementsAre(10));
  EXPECT_THAT(FindEntryIndexes(40, Map), testing::ElementsAre());
}
