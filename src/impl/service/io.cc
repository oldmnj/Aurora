module;

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <ios>
#include <limits>
#include <sstream>
#include <system_error>

module aurora.service;
import launcher.base;
import :random;

namespace launcher {
auto IO::Exists(Path path) -> Result<bool> {
    return Ok(std::filesystem::exists(path));
}

auto IO::ReadFile(Path path) -> Result<Bytes> {
    auto status = std::filesystem::status(path);
    if (Exists(path)) {
        if (std::filesystem::is_regular_file(status)) {
            return Err<Bytes>(
                    {ErrorCategory::IO, ErrorCode::IOError,
                     "Failed to read: path is a directory, not a file"}
            );
        }
        return Err<Bytes>(
                {ErrorCategory::IO, ErrorCode::FileNotFound,
                 fmt::format("File {} not found", path.string())}
        );
    }
    try {
        auto f_size             = std::filesystem::file_size(path);
        std::uintmax_t max_size = 67'108'864;  // 64MIB
        if (f_size >= max_size) {
            // Warn: file size is so big, use ifstream
            return Err<Bytes>(
                    {ErrorCategory::IO, ErrorCode::FileTooLarge,
                     "File is so large, should ise ifstream"}
            );
        }
        std::ifstream file{path, std::ios::binary};
        if (!file.is_open()) {
            return Err<Bytes>(
                    {ErrorCategory::IO, ErrorCode::IOError,
                     "cannot open file {}"}
            );
        }
        Bytes buffer{static_cast<size_t>(f_size)};
        file.read(
                reinterpret_cast<char *>(buffer.data()),
                static_cast<std::streamsize>(f_size)
        );

        if (file.gcount() != static_cast<std::streamsize>(f_size)) {
            return Err<Bytes>(
                    {ErrorCategory::IO, ErrorCode::IOError,
                     "failed to read file"}
            );
        }

        return Ok<Bytes>({std::move(buffer)});
    } catch (std::filesystem::filesystem_error &e) {
        if (e.code() == std::errc::permission_denied) {
            return Err<Bytes>(
                    {ErrorCategory::IO, ErrorCode::PermissionDenied,
                     fmt::format("permission denied: {}", e.what())}
            );
        }
        return Err<Bytes>(
                {ErrorCategory::IO, ErrorCode::IOError, "cannot return bytes"}
        );
    }
}

auto IO::WriteFile(Path path, ConstByteSpan data) -> Result<void> {
    try {
        if (auto res = Mkdirs(path); !res) {
            return Err<void>(std::move(res.Error()));
        }
        std::ofstream file{path.string(), std::ios::binary};

        if (!file.is_open()) {
            return Err<void>(
                    {ErrorCategory::IO, ErrorCode::PermissionDenied,
                     "Cannot open the file"}
            );
        }

        file.write(
                reinterpret_cast<const char *>(data.data()),
                static_cast<std::streamsize>(data.size())
        );

        if (!file.good()) {
            return Err<void>(
                    {ErrorCategory::IO, ErrorCode::IOError,
                     "Cannot write data into file"}
            );
        }

        return Ok();

    } catch (const std::exception &e) {
        return Err<void>(
                {ErrorCategory::IO, ErrorCode::IOError,
                 fmt::format("Failed to write into file: {}", e.what())}
        );
    }
}

auto IO::WriteFileAtomic(Path path, ConstByteSpan data) -> Result<void> {
    auto uuid_result = UUIDGenerator::V4();
    if (uuid_result.HasError()) {
        return Err<void>(
                {ErrorCategory::IO, ErrorCode::IOError,
                 uuid_result.Error().Message()}
        );
    }
    auto tmp = path.string() + ".tmp." + uuid_result.Value().ToString();

    if (auto res = WriteFile(tmp, data); !res) {
        return Err<void>(
                {ErrorCategory::IO, ErrorCode::IOError,
                 "Cannot write data into tmp file"}
        );
    }

    try {
        std::filesystem::rename(tmp, path);
        return Ok();
    } catch (std::filesystem::filesystem_error &e) {
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return Err<void>(
                {ErrorCategory::IO, ErrorCode::IOError,
                 fmt::format("Failed to write into file: {}", e.what())}
        );
    }
}

auto IO::FileSize(Path path) -> Result<u64> {
    try {
        auto file_size = std::filesystem::file_size(path);
        if (file_size > std::numeric_limits<u64>::max()) {
            return Err<u64>(
                    {ErrorCategory::IO, ErrorCode::FileTooLarge,
                     "File is too big"}
            );
        }
        return static_cast<u64>(file_size);
    } catch (std::filesystem::filesystem_error &e) {
        return Err<u64>(
                {ErrorCategory::IO, ErrorCode::IOError,
                 fmt::format("Failed to get file size", e.what())}
        );
    }
}

auto IO::ReadText(Path path) -> Result<String> {
    try {
        if (!std::filesystem::exists(path)) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::FileNotFound,
                     "cannot find file or directory"}
            );
        }
        if (std::filesystem::is_directory(path)) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::FileNotFound,
                     "This is a directory"}
            );
        }
        auto size = std::filesystem::file_size(path);

        if (size > 67'108'864) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::FileTooLarge,
                     "file is too large"}
            );
        }

        std::ifstream file(path);

        if (!file.is_open()) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::PermissionDenied,
                     "Failed to open text file"}
            );
        }

        std::stringstream ss;
        ss << file.rdbuf();

        if (!file.good() && !file.eof()) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::IOError,
                     "Failed to get text"}
            );
        }

        return ss.str();
    } catch (std::filesystem::filesystem_error &e) {
        if (e.code() == std::errc::permission_denied) {
            return Err<String>(
                    {ErrorCategory::IO, ErrorCode::PermissionDenied,
                     fmt::format("Cannot get text: ", e.what())}
            );
        }
        return Err<String>(
                {ErrorCategory::IO, ErrorCode::IOError, "Failed to get text"}
        );
    }
}
}  // namespace launcher
