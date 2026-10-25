#include "band_layout.h"

#include <array>
#include <cstddef>
#include <iostream>

namespace {

int failures = 0;

template <typename Actual, typename Expected>
void expectEqual(const Actual& actual, const Expected& expected,
                 const char* testName)
{
    if (actual == expected)
        return;

    std::cerr << "FAIL: " << testName << '\n';
    ++failures;
}

void testFixedWidths()
{
    expectEqual(splix::fixedBandWidthBytes(300), 310UL,
                "300 DPI fixed width");
    expectEqual(splix::fixedBandWidthBytes(600), 620UL,
                "600 DPI fixed width");
    expectEqual(splix::fixedBandWidthBytes(1200), 1240UL,
                "1200 DPI fixed width");
}

void testRowMajorA5Padding()
{
    const std::array<unsigned char, 10> source = {
        0xee, 0x11, 0x12, 0x13, 0x14,
        0xef, 0x21, 0x22, 0x23, 0x24,
    };
    std::array<unsigned char, 12> band;
    band.fill(0xa5);

    splix::copyBandRows(band.data(), 6, 2, source.data(), 5, 1, 2,
                        splix::RowMajor);

    const std::array<unsigned char, 12> expected = {
        0x11, 0x12, 0x13, 0x14, 0x00, 0x00,
        0x21, 0x22, 0x23, 0x24, 0x00, 0x00,
    };
    expectEqual(band, expected, "A5 rows are left-aligned and zero-padded");
}

void testColumnMajorA5Padding()
{
    const std::array<unsigned char, 10> source = {
        0xee, 0x11, 0x12, 0x13, 0x14,
        0xef, 0x21, 0x22, 0x23, 0x24,
    };
    std::array<unsigned char, 12> band;
    band.fill(0xa5);

    splix::copyBandRows(band.data(), 6, 2, source.data(), 5, 1, 2,
                        splix::ColumnMajor);

    const std::array<unsigned char, 12> expected = {
        0x11, 0x21, 0x12, 0x22, 0x13, 0x23,
        0x14, 0x24, 0x00, 0x00, 0x00, 0x00,
    };
    expectEqual(band, expected, "transposed rows preserve zero padding");
}

void testPartialBandIsFullyInitialized()
{
    const std::array<unsigned char, 4> source = {0x31, 0x32, 0x41, 0x42};
    std::array<unsigned char, 9> band;
    band.fill(0xa5);

    splix::copyBandRows(band.data(), 3, 3, source.data(), 2, 0, 2,
                        splix::RowMajor);

    const std::array<unsigned char, 9> expected = {
        0x31, 0x32, 0x00,
        0x41, 0x42, 0x00,
        0x00, 0x00, 0x00,
    };
    expectEqual(band, expected, "partial bands contain no stale bytes");
}

void testMatchingWidthPreservesProtocolBytes()
{
    const std::array<unsigned char, 6> source = {
        0x51, 0x52, 0x53, 0x61, 0x62, 0x63,
    };
    std::array<unsigned char, 6> band;
    band.fill(0xa5);

    splix::copyBandRows(band.data(), 3, 2, source.data(), 3, 0, 2,
                        splix::RowMajor);

    expectEqual(band, source, "matching widths preserve protocol bytes");
}

void testOversizedMarginProducesBlankBand()
{
    const std::array<unsigned char, 4> source = {0x11, 0x12, 0x21, 0x22};
    std::array<unsigned char, 6> band;
    band.fill(0xa5);

    splix::copyBandRows(band.data(), 3, 2, source.data(), 2, 2, 2,
                        splix::RowMajor);

    const std::array<unsigned char, 6> expected = {};
    expectEqual(band, expected, "oversized margins cannot underflow copy width");
}

} // namespace

int main()
{
    testFixedWidths();
    testRowMajorA5Padding();
    testColumnMajorA5Padding();
    testPartialBandIsFullyInitialized();
    testMatchingWidthPreservesProtocolBytes();
    testOversizedMarginProducesBlankBand();

    if (failures != 0)
        return 1;

    std::cout << "fixed-bandwidth tests passed\n";
    return 0;
}
