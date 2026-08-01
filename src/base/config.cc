module;

#include <chrono>
#include <expected>
// #include <filesystem>
// #include <source_location>

module launcher.base;

namespace launcher {

Result<void> Config::Validate() const {
    if (this->path.cache_directory.empty() || this->path.log_directory.empty() ||
            this->path.runtime_directory.empty() || this->path.temp_directory.empty() 
        /*||
            !std::filesystem::exists(this->path.cache_directory) ||
            !std::filesystem::exists(this->path.runtime_directory) ||
            !std::filesystem::exists(this->path.log_directory) ||
            !std::filesystem::exists(this->path.temp_directory)
    */) {
        return std::unexpected<Error>(
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument, "路径参数无效"});
    } else if (this->network.timeout <= std::chrono::seconds{0}) {
        return std::unexpected<Error>{
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument, "超时参数不是正数"}
        };
    } else if (this->runtime.worker_threads <= 0) {
        return std::unexpected<Error>{
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument,
                      "runtime: 线程数不能为非正整数"}
        };
    } else {
        return {};
    }
    return std::unexpected<Error>{
            Error{ErrorCategory::Config, ErrorCode::InternalError, "未知错误"}
    };
}

void Config::Reset() {
    this->path    = PathConfig{};
    this->runtime = RuntimeConfig{};
    this->network = NetworkConfig{};
    this->logger  = LoggerConfig{};
}

}  // namespace launcher
