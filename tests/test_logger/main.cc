import launcher;


int main(void) {
    launcher::LoggerConfig config{};
    launcher::Logger::Initialize(config);
    launcher::Logger::Info("Hello, {}!", "World");
    return 0;
}
