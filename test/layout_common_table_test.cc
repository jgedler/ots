// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <vector>
#include <gtest/gtest.h>

#include "layout.h"
#include "ots-memory-stream.h"

namespace {

const uint32_t kFakeTag = 0x00000000;
const size_t kScriptRecordSize = 6;
const size_t kLangSysRecordSize = 6;

bool BuildFakeScriptListTable(ots::OTSStream *out, const uint16_t script_count,
                              const uint16_t langsys_count,
                              const uint16_t feature_count) {
  if (!out->WriteU16(script_count)) {
    return false;
  }
  const off_t script_record_end = out->Tell() +
      kScriptRecordSize * script_count;
  const size_t script_table_size = 4 + kLangSysRecordSize * langsys_count;
  for (unsigned i = 0; i < script_count; ++i) {
    if (!out->WriteU32(kFakeTag) ||
        !out->WriteU16(script_record_end + i * script_table_size)) {
      return false;
    }
  }

  // Offsets to LangSys tables are measured from the beginning of each
  // script table.
  const off_t langsys_record_end = 4 + kLangSysRecordSize * langsys_count;
  const size_t langsys_table_size = 6 + 2 * feature_count;
  // Write Fake Script tables.
  for (unsigned i = 0; i < script_count; ++i) {
    if (!out->WriteU16(0x0000) ||
        !out->WriteU16(langsys_count)) {
      return false;
    }
    for (unsigned j = 0; j < langsys_count; ++j) {
      if (!out->WriteU32(kFakeTag) ||
          !out->WriteU16(langsys_record_end + j * langsys_table_size)) {
        return false;
      }
    }
  }

  // Write Fake LangSys tables.
  for (unsigned i = 0; i < langsys_count; ++i) {
    if (!out->WriteU16(0x0000) ||
        !out->WriteU16(0xFFFF) ||
        !out->WriteU16(feature_count)) {
      return false;
    }
    for (unsigned j = 0; j < feature_count; ++j) {
      if (!out->WriteU16(j)) {
        return false;
      }
    }
  }
  return true;
}

const size_t kFeatureRecordSize = 6;

bool BuildFakeFeatureListTable(ots::OTSStream *out,
                               const uint16_t feature_count,
                               const uint16_t lookup_count) {
  if (!out->WriteU16(feature_count)) {
    return false;
  }
  const off_t feature_record_end = out->Tell() +
      kFeatureRecordSize * feature_count;
  const size_t feature_table_size = 4 + 2 * lookup_count;
  for (unsigned i = 0; i < feature_count; ++i) {
    if (!out->WriteU32(kFakeTag) ||
        !out->WriteU16(feature_record_end + i * feature_table_size)) {
      return false;
    }
  }

  // Write FeatureTable
  for (unsigned i = 0; i < feature_count; ++i) {
    if (!out->WriteU16(0x0000) ||
        !out->WriteU16(lookup_count)) {
      return false;
    }
    for (uint16_t j = 0; j < lookup_count; ++j) {
      if (!out->WriteU16(j)) {
        return false;
      }
    }
  }
  return true;
}

bool BuildFakeLookupListTable(ots::OTSStream *out, const uint16_t lookup_count,
                              const uint16_t subtable_count) {
  if (!out->WriteU16(lookup_count)) {
    return false;
  }
  const off_t base_offset_lookup = out->Tell();
  if (!out->Pad(2 * lookup_count)) {
    return false;
  }

  std::vector<off_t> offsets_lookup(lookup_count, 0);
  for (uint16_t i = 0; i < lookup_count; ++i) {
    offsets_lookup[i] = out->Tell();
    if (!out->WriteU16(i + 1) ||
        !out->WriteU16(0) ||
        !out->WriteU16(subtable_count) ||
        !out->Pad(2 * subtable_count) ||
        !out->WriteU16(0)) {
      return false;
    }
  }

  const off_t offset_lookup_table_end = out->Tell();
  // Allocate 256 bytes for each subtable.
  if (!out->Pad(256 * lookup_count * subtable_count)) {
    return false;
  }

  if (!out->Seek(base_offset_lookup)) {
    return false;
  }
  for (unsigned i = 0; i < lookup_count; ++i) {
    if (!out->WriteU16(offsets_lookup[i])) {
      return false;
    }
  }

  for (unsigned i = 0; i < lookup_count; ++i) {
    if (!out->Seek(offsets_lookup[i] + 6)) {
      return false;
    }
    for (unsigned j = 0; j < subtable_count; ++j) {
      if (!out->WriteU16(offset_lookup_table_end +
                         256*i*subtable_count + 256*j)) {
        return false;
      }
    }
  }
  return true;
}

bool BuildFakeCoverageFormat1(ots::OTSStream *out, const uint16_t glyph_count) {
  if (!out->WriteU16(1) || !out->WriteU16(glyph_count)) {
    return false;
  }
  for (uint16_t glyph_id = 1; glyph_id <= glyph_count; ++glyph_id) {
    if (!out->WriteU16(glyph_id)) {
      return false;
    }
  }
  return true;
}

bool BuildFakeCoverageFormat2(ots::OTSStream *out, const uint16_t range_count) {
  if (!out->WriteU16(2) || !out->WriteU16(range_count)) {
    return false;
  }
  uint16_t glyph_id = 1;
  uint16_t start_coverage_index = 0;
  for (unsigned i = 0; i < range_count; ++i) {
    // Write consecutive ranges in which each range consists of two glyph id.
    if (!out->WriteU16(glyph_id) ||
        !out->WriteU16(glyph_id + 1) ||
        !out->WriteU16(start_coverage_index)) {
      return false;
    }
    glyph_id += 2;
    start_coverage_index += 2;
  }
  return true;
}

bool BuildFakeClassDefFormat1(ots::OTSStream *out, const uint16_t glyph_count) {
  if (!out->WriteU16(1) ||
      !out->WriteU16(1) ||
      !out->WriteU16(glyph_count)) {
    return false;
  }
  for (uint16_t class_value = 1; class_value <= glyph_count; ++class_value) {
    if (!out->WriteU16(class_value)) {
      return false;
    }
  }
  return true;
}

bool BuildFakeClassDefFormat2(ots::OTSStream *out, const uint16_t range_count) {
  if (!out->WriteU16(2) || !out->WriteU16(range_count)) {
    return false;
  }
  uint16_t glyph_id = 1;
  for (uint16_t class_value = 1; class_value <= range_count; ++class_value) {
    // Write consecutive ranges in which each range consists of one glyph id.
    if (!out->WriteU16(glyph_id) ||
        !out->WriteU16(glyph_id + 1) ||
        !out->WriteU16(class_value)) {
      return false;
    }
    glyph_id += 2;
  }
  return true;
}

bool BuildFakeDeviceTable(ots::OTSStream *out, const uint16_t start_size,
                          const uint16_t end_size, const uint16_t format) {
  if (!out->WriteU16(start_size) ||
      !out->WriteU16(end_size) ||
      !out->WriteU16(format)) {
    return false;
  }

  const unsigned num_values = std::abs(end_size - start_size) + 1;
  const unsigned num_bits = (1 << format) * num_values;
  const unsigned num_units = (num_bits - 1) / 16 + 1;
  if (!out->Pad(num_units * 2)) {
    return false;
  }
  return true;
}

class TestStream : public ots::MemoryStream {
 public:
  TestStream()
      : ots::MemoryStream(data_, sizeof(data_)), size_(0) {
    std::memset(reinterpret_cast<char*>(data_), 0, sizeof(data_));
  }

  uint8_t* data() { return data_; }
  size_t size() const { return size_; }

  virtual bool WriteRaw(const void *data, size_t length) {
    if (Tell() + length > size_) {
      size_ = Tell() + length;
    }
    return ots::MemoryStream::WriteRaw(data, length);
  }

 private:
  size_t size_;
  uint8_t data_[4096];
};

class ScriptListTableTest : public ::testing::Test {
 protected:

  TestStream out;
  ots::OpenTypeFile file;
};

class FeatureListTableTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    num_features = 0;
  }

  TestStream out;
  ots::OpenTypeFile file;
  uint16_t num_features;
};

bool fakeLookupParser(const ots::OpenTypeFile*, const uint8_t *, const size_t) {
  return true;
}

// Lookup Type parsers doesn't check anything.
const ots::LookupTypeParser FakeLookupParsers[] = {
  {1, fakeLookupParser},
  {2, fakeLookupParser},
  {3, fakeLookupParser},
  {4, fakeLookupParser},
  {5, fakeLookupParser}
};

class LookupListTableTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    num_lookups = 0;
  }

  bool Parse() {
    return ots::ParseLookupListTable(&file, out.data(), out.size(), 5,
                                     FakeLookupParsers, &num_lookups);
  }

  TestStream out;
  ots::OpenTypeFile file;
  uint16_t num_lookups;
};

}  // namespace

TEST_F(ScriptListTableTest, TestSuccess) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  EXPECT_TRUE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestBadScriptCount) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set too large script count.
  out.Seek(0);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestScriptRecordOffsetUnderflow) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set bad offset to ScriptRecord[0].
  out.Seek(6);
  out.WriteU16(0);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestScriptRecordOffsetOverflow) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set bad offset to ScriptRecord[0].
  out.Seek(6);
  out.WriteU16(out.size());
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestBadLangSysCount) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set too large langsys count.
  out.Seek(10);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestLangSysRecordOffsetUnderflow) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set bad offset to LangSysRecord[0].
  out.Seek(16);
  out.WriteU16(0);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestLangSysRecordOffsetOverflow) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set bad offset to LangSysRecord[0].
  out.Seek(16);
  out.WriteU16(out.size());
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestBadReqFeatureIndex) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set too large feature index to ReqFeatureIndex of LangSysTable[0].
  out.Seek(20);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestBadFeatureCount) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set too large feature count to LangSysTable[0].
  out.Seek(22);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(ScriptListTableTest, TestBadFeatureIndex) {
  BuildFakeScriptListTable(&out, 1, 1, 1);
  // Set too large feature index to ReatureIndex[0] of LangSysTable[0].
  out.Seek(24);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseScriptListTable(out.data(), out.size(), 1));
}

TEST_F(FeatureListTableTest, TestSuccess) {
  BuildFakeFeatureListTable(&out, 1, 1);
  EXPECT_TRUE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                         &num_features));
  EXPECT_EQ(num_features, 1);
}

TEST_F(FeatureListTableTest, TestSuccess2) {
  BuildFakeFeatureListTable(&out, 5, 1);
  EXPECT_TRUE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                         &num_features));
  EXPECT_EQ(num_features, 5);
}

TEST_F(FeatureListTableTest, TestBadFeatureCount) {
  BuildFakeFeatureListTable(&out, 1, 1);
  // Set too large feature count.
  out.Seek(0);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                          &num_features));
}

TEST_F(FeatureListTableTest, TestOffsetFeatureUnderflow) {
  BuildFakeFeatureListTable(&out, 1, 1);
  // Set bad offset to FeatureRecord[0].
  out.Seek(6);
  out.WriteU16(0);
  EXPECT_FALSE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                          &num_features));
}

TEST_F(FeatureListTableTest, TestOffsetFeatureOverflow) {
  BuildFakeFeatureListTable(&out, 1, 1);
  // Set bad offset to FeatureRecord[0].
  out.Seek(6);
  out.WriteU16(out.size());
  EXPECT_FALSE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                          &num_features));
}

TEST_F(FeatureListTableTest, TestBadLookupCount) {
  BuildFakeFeatureListTable(&out, 1, 1);
  // Set too large lookup count to FeatureTable[0].
  out.Seek(10);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseFeatureListTable(out.data(), out.size(), 1,
                                          &num_features));
}

TEST_F(LookupListTableTest, TestSuccess) {
  BuildFakeLookupListTable(&out, 1, 1);
  EXPECT_TRUE(Parse());
  EXPECT_EQ(num_lookups, 1);
}

TEST_F(LookupListTableTest, TestSuccess2) {
  BuildFakeLookupListTable(&out, 5, 1);
  EXPECT_TRUE(Parse());
  EXPECT_EQ(num_lookups, 5);
}

TEST_F(LookupListTableTest, TestOffsetLookupTableUnderflow) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set bad offset to Lookup[0].
  out.Seek(2);
  out.WriteU16(0);
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TestOffsetLookupTableOverflow) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set bad offset to Lookup[0].
  out.Seek(2);
  out.WriteU16(out.size());
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TestOffsetSubtableUnderflow) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set bad offset to SubTable[0] of LookupTable[0].
  out.Seek(10);
  out.WriteU16(0);
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TestOffsetSubtableOverflow) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set bad offset to SubTable[0] of LookupTable[0].
  out.Seek(10);
  out.WriteU16(out.size());
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TesBadLookupCount) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set too large lookup count of LookupTable[0].
  out.Seek(0);
  out.WriteU16(2);
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TesBadLookupType) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set too large lookup type of LookupTable[0].
  out.Seek(4);
  out.WriteU16(6);
  EXPECT_FALSE(Parse());
}

TEST_F(LookupListTableTest, TesBadSubtableCount) {
  BuildFakeLookupListTable(&out, 1, 1);
  // Set too large sutable count of LookupTable[0].
  out.Seek(8);
  out.WriteU16(2);
  EXPECT_FALSE(Parse());
}

TEST(CoverageTableTest, TestSuccessFormat1) {
  TestStream out;
  BuildFakeCoverageFormat1(&out, 1);
  EXPECT_TRUE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageTableTest, TestSuccessFormat2) {
  TestStream out;
  BuildFakeCoverageFormat2(&out, 1);
  EXPECT_TRUE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageTableTest, TestBadFormat) {
  TestStream out;
  BuildFakeCoverageFormat1(&out, 1);
  // Set bad format.
  out.Seek(0);
  out.WriteU16(3);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageFormat1Test, TestBadGlyphCount) {
  TestStream out;
  BuildFakeCoverageFormat1(&out, 1);
  // Set too large glyph count.
  out.Seek(2);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageFormat1Test, TestBadGlyphId) {
  TestStream out;
  BuildFakeCoverageFormat1(&out, 1);
  // Set too large glyph id.
  out.Seek(4);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageFormat2Test, TestBadRangeCount) {
  TestStream out;
  BuildFakeCoverageFormat2(&out, 1);
  // Set too large range count.
  out.Seek(2);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageFormat2Test, TestBadRange) {
  TestStream out;
  BuildFakeCoverageFormat2(&out, 1);
  // Set reverse order glyph id to start/end fields.
  out.Seek(4);
  out.WriteU16(2);
  out.WriteU16(1);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 1));
}

TEST(CoverageFormat2Test, TestRangeOverlap) {
  TestStream out;
  BuildFakeCoverageFormat2(&out, 2);
  // Set overlapping glyph id to an end field.
  out.Seek(12);
  out.WriteU16(1);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 2));
}

TEST(CoverageFormat2Test, TestRangeOverlap2) {
  TestStream out;
  BuildFakeCoverageFormat2(&out, 2);
  // Set overlapping range.
  out.Seek(10);
  out.WriteU16(1);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseCoverageTable(out.data(), out.size(), 2));
}

TEST(ClassDefTableTest, TestSuccessFormat1) {
  TestStream out;
  BuildFakeClassDefFormat1(&out, 1);
  EXPECT_TRUE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefTableTest, TestSuccessFormat2) {
  TestStream out;
  BuildFakeClassDefFormat2(&out, 1);
  EXPECT_TRUE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefTableTest, TestBadFormat) {
  TestStream out;
  BuildFakeClassDefFormat1(&out, 1);
  // Set bad format.
  out.Seek(0);
  out.WriteU16(3);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat1Test, TestBadStartGlyph) {
  TestStream out;
  BuildFakeClassDefFormat1(&out, 1);
  // Set too large start glyph id.
  out.Seek(2);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat1Test, TestBadGlyphCount) {
  TestStream out;
  BuildFakeClassDefFormat1(&out, 1);
  // Set too large glyph count.
  out.Seek(4);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat1Test, TestBadClassValue) {
  TestStream out;
  BuildFakeClassDefFormat1(&out, 1);
  // Set too large class value.
  out.Seek(6);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat2Test, TestBadRangeCount) {
  TestStream out;
  BuildFakeClassDefFormat2(&out, 1);
  // Set too large range count.
  out.Seek(2);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat2Test, TestRangeOverlap) {
  TestStream out;
  BuildFakeClassDefFormat2(&out, 2);
  // Set overlapping glyph id to an end field.
  out.Seek(12);
  out.WriteU16(1);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(ClassDefFormat2Test, TestRangeOverlap2) {
  TestStream out;
  BuildFakeClassDefFormat2(&out, 2);
  // Set overlapping range.
  out.Seek(10);
  out.WriteU16(1);
  out.WriteU16(2);
  EXPECT_FALSE(ots::ParseClassDefTable(out.data(), out.size(), 1, 1));
}

TEST(DeviceTableTest, TestDeltaFormat1Success) {
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 1);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 9, 1);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
}

TEST(DeviceTableTest, TestDeltaFormat1Fail) {
  // Pass shorter length than expected.
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 1);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 9, 1);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
}

TEST(DeviceTableTest, TestDeltaFormat2Success) {
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 1, 2);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 2);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
}

TEST(DeviceTableTest, TestDeltaFormat2Fail) {
  // Pass shorter length than expected.
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 2);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 9, 2);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
}

TEST(DeviceTableTest, TestDeltaFormat3Success) {
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 1, 3);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 3);
    EXPECT_TRUE(ots::ParseDeviceTable(out.data(), out.size()));
  }
}

TEST(DeviceTableTest, TestDeltaFormat3Fail) {
  // Pass shorter length than expected.
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 8, 3);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
  {
    TestStream out;
    BuildFakeDeviceTable(&out, 1, 9, 3);
    EXPECT_FALSE(ots::ParseDeviceTable(out.data(), out.size() - 1));
  }
}

