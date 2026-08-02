import launcher;


int main(void) {
    launcher::LoggerConfig config{};
    launcher::Logger::Initialize(config);
    launcher::Error err{launcher::ErrorCategory::None, launcher::ErrorCode::ParseError, "测试错误"};
    launcher::Logger::Info("Hello, {}!", "World");
    launcher::Logger::Error("{}", err.ToString());
    return 0;
}
