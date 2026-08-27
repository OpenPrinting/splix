SP::Result<std::unique_ptr<BandPlane>> Algo0x11::compress([[maybe_unused]] const Request& request, std::span<const uint8_t> data,
        uint32_t width, uint32_t height)
{
    // Input sanitization
    if (width > 32768 || height > 32768 || width == 0 || height == 0) {
        ERRORMSG(_("Invalid raster dimensions for Algo0x11: %ux%u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    uint64_t expectedSize = (static_cast<uint64_t>(width) * height) / 8;
    if (data.size() < expectedSize) {
        ERRORMSG(_("Input data too small for Algo0x11: got %zu, expected %llu"),
            data.size(), static_cast<unsigned long long>(expectedSize));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Safety limit on output size (512MB)
    if (expectedSize > 512 * 1024 * 1024) {
        ERRORMSG(_("Output size too large for security safety (0x11)"));
        return SP::Unexpected(SP::Error::MemoryError);
    }

    if (data.empty()) {
        ERRORMSG(_("Invalid given data for compression (0x11)"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Lookup for the best occurs
    auto lookupRes = _lookupBestOccurs(data);
    if (!lookupRes) {
        return SP::Unexpected(lookupRes.error());
    }

    std::vector<uint8_t> output;
    auto compressRes = _compress(data, output);
    if (!compressRes) {
        return SP::Unexpected(compressRes.error());
    }

    // Register the result into a band plane
    auto plane = std::make_unique<BandPlane>();
    const size_t outputSize = output.size();
    plane->setData(std::move(output));
    plane->setEndian(BandPlane::Endian::Dependant);
    plane->setCompression(0x11);

    DEBUGMSG(_("Finished band encoding: type=0x11, size=%zu"), outputSize);

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
