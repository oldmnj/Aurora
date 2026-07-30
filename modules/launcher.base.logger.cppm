module;

export module launcher.base:logger;
import :error;
import :types;
import :config;

namespace launcher {
export class Logger {
  public:
    static Result<void> Initialize(const LoggerConfig &config);
};
}  // namespace launcher
