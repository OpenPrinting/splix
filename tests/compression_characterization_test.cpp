#include <gtest/gtest.h>

#include "algo0x0d.h"
#include "algo0x0e.h"
#include "algo0x15.h"
#include "bandplane.h"
#include "request.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace {
constexpr uint32_t kWidth = 8;
constexpr uint32_t kBandHeight = 64;

TEST(CompressionCharacterization, Algo0x0DSinglePixelSnapshot) {
    std::vector<uint8_t> raster(kBandHeight, 0);
    raster.front() = 0x80;

    auto result = Algo0x0D{}.compress(Request{}, raster, kWidth, kBandHeight);

    ASSERT_TRUE(result.has_value());
    const std::array<uint8_t, 4> expected{0x01, 0x00, 0x00, 0x00};
    EXPECT_TRUE(std::ranges::equal(result.value()->data_span(), expected));
    EXPECT_EQ(result.value()->compression(), 0x0d);
    EXPECT_EQ(result.value()->endian(), BandPlane::Endian::Dependant);
}

TEST(CompressionCharacterization, Algo0x0EBlankBandSnapshot) {
    const std::vector<uint8_t> raster(kBandHeight, 0xff);

    auto result = Algo0x0E{}.compress(Request{}, raster, kWidth, kBandHeight);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value()->dataSize(), kBandHeight * 3);
    const auto encoded = result.value()->data_span();
    const std::array<uint8_t, 3> expectedPacket{0xfe, 0xcd, 0xff};
    for (size_t offset = 0; offset < encoded.size(); offset += 3) {
        EXPECT_TRUE(std::ranges::equal(encoded.subspan(offset, 3),
                                       expectedPacket));
    }
    EXPECT_EQ(result.value()->compression(), 0x0e);
    EXPECT_EQ(result.value()->endian(), BandPlane::Endian::Dependant);
}

#ifndef DISABLE_JBIG
TEST(CompressionCharacterization, Algo0x15IsBitDeterministic) {
    const std::vector<uint8_t> raster(kBandHeight, 0x5a);
    Algo0x15 first;
    Algo0x15 second;

    auto firstResult = first.compress(Request{}, raster, kWidth, kBandHeight);
    auto secondResult = second.compress(Request{}, raster, kWidth, kBandHeight);

    ASSERT_TRUE(firstResult.has_value());
    ASSERT_TRUE(secondResult.has_value());
    EXPECT_TRUE(std::ranges::equal(firstResult.value()->data_span(),
                                   secondResult.value()->data_span()));
    EXPECT_TRUE(std::equal(first.getBIHdata(), first.getBIHdata() + 20,
                           second.getBIHdata()));
    EXPECT_FALSE(firstResult.value()->data_span().empty());
    EXPECT_EQ(firstResult.value()->compression(), 0x15);
    EXPECT_EQ(firstResult.value()->endian(), BandPlane::Endian::BigEndian);
}
#endif

TEST(CompressionCharacterization, AlgorithmsRejectEmptyRaster) {
    const std::vector<uint8_t> empty;
    const Request request;

    EXPECT_EQ(Algo0x0D{}.compress(request, empty, 0, 0).error(),
              SP::Error::InvalidArgument);
    EXPECT_EQ(Algo0x0E{}.compress(request, empty, 0, 0).error(),
              SP::Error::InvalidArgument);
#ifndef DISABLE_JBIG
    EXPECT_EQ(Algo0x15{}.compress(request, empty, 0, 0).error(),
              SP::Error::InvalidArgument);
#endif
}
}  // namespace
