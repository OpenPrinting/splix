/*
 *      sp_result.h
 *
 *  Structured error handling for SpliX using C++23 std::expected.
 */
#ifndef _SP_RESULT_H_
#define _SP_RESULT_H_

#include <expected>
#include <string_view>
#include <system_error>

namespace SP {

/**
 * @brief core error codes for the SpliX driver
 */
enum class Error {
    None = 0,
    Generic,
    InconsistentData,
    IOError,
    MemoryError,
    CompressionError,
    Unsupported,
    PrinterError,
    EndOfJob,
    RasterOpenError,
    RasterReadError,
    PPDOpenError,
    PPDVersionMismatch,
    LogicError,
    InvalidArgument,
    RasterDimensionTooLarge,
    InvalidState,
    SerializationError
};

/**
 * @brief A standard result type for operations that can fail.
 */
template <typename T = void>
using Result = std::expected<T, Error>;

/**
 * @brief Helper to create an unexpected result.
 */
inline auto Unexpected(Error err) {
    return std::unexpected<Error>(err);
}

/**
 * @brief Helper to convert an Error enum to a human-readable string.
 */
constexpr std::string_view to_string(Error err) {
    switch (err) {
        case Error::None:             return "No error";
        case Error::Generic:          return "Generic error";
        case Error::InconsistentData: return "Inconsistent data";
        case Error::IOError:          return "I/O error";
        case Error::MemoryError:       return "Memory allocation error";
        case Error::CompressionError:  return "Compression failed";
        case Error::Unsupported:      return "Unsupported feature";
        case Error::PrinterError:     return "Printer reported error";
        case Error::EndOfJob:         return "End of job reached";
        case Error::RasterOpenError:  return "Failed to open CUPS raster";
        case Error::RasterReadError:  return "Failed to read from CUPS raster";
        case Error::PPDOpenError:      return "Failed to open PPD file";
        case Error::PPDVersionMismatch: return "PPD version mismatch";
        case Error::LogicError:         return "Internal logic error";
        case Error::InvalidArgument:    return "Invalid argument provided";
        case Error::RasterDimensionTooLarge: return "Raster dimension too large";
        case Error::InvalidState:       return "Invalid object state";
        case Error::SerializationError: return "Failed to serialize/deserialize data";
        default:                      return "Unknown error";
    }
}

} // namespace SP

#endif /* _SP_RESULT_H_ */
