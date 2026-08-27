#include <gtest/gtest.h>

#include "page.h"
#include "qpdl.h"
#include "request.h"
#include "sp_portable.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace {
std::vector<uint8_t> captureRenderedPage(const Request& request, Page& page) {
    int outputPipe[2];
    if (pipe(outputPipe) != 0) return {};
    const int savedStdout = dup(SP_STDOUT_FILENO);
    if (savedStdout < 0 || dup2(outputPipe[1], SP_STDOUT_FILENO) < 0) {
        close(outputPipe[0]);
        close(outputPipe[1]);
        return {};
    }
    close(outputPipe[1]);

    const auto result = renderPage(request, &page, true);
    dup2(savedStdout, SP_STDOUT_FILENO);
    close(savedStdout);
    if (!result) {
        close(outputPipe[0]);
        return {};
    }

    std::vector<uint8_t> output;
    std::array<uint8_t, 64> buffer{};
    for (ssize_t count = read(outputPipe[0], buffer.data(), buffer.size());
         count > 0;
         count = read(outputPipe[0], buffer.data(), buffer.size())) {
        output.insert(output.end(), buffer.begin(), buffer.begin() + count);
    }
    close(outputPipe[0]);
    return output;
}

TEST(ProtocolCharacterization, EmptyPageHeaderAndTrailerSnapshot) {
    Request request;
    request.setDuplex(Request::Simplex);
    Page page;
    page.setXResolution(600);
    page.setYResolution(600);
    page.setWidth(0x0123);
    page.setHeight(0x0040);
    page.setCopiesNr(2);
    page.setCompression(0x11);

    const std::array<uint8_t, 20> expected{
        0x00, 0x06, 0x00, 0x02, 0x00, 0x01, 0x23, 0x00, 0x40, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x00, 0x02};
    EXPECT_EQ(captureRenderedPage(request, page),
              (std::vector<uint8_t>{expected.begin(), expected.end()}));
}
}  // namespace
