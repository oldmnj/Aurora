/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

export module aurora.service:io;
import launcher.base;


namespace launcher {
export class IO {
  public:
    static auto ReadFile(Path) -> Result<Bytes>;

    static auto WriteFile(Path, ConstByteSpan) -> Result<void>;

    static auto WriteFileAtomic(Path, ConstByteSpan) -> Result<void>;

    static auto ReadText(Path) -> Result<String>;

    static auto WriteText(Path, StringView) -> Result<void>;

    static auto OpenFile(Path) -> Result<std::ifstream>;

    static auto OpenText(Path) -> Result<std::ifstream>;

    static auto CreateFile(Path) -> Result<std::ofstream>;

    static auto CreateText(Path) -> Result<std::ofstream>;

    static auto AppendFile(Path) -> Result<std::ofstream>;

    static auto AppendText(Path) -> Result<std::ofstream>;

    static auto Mkdirs(Path) -> Result<void>;

    static auto RemoveAll(Path) -> Result<void>;

    static auto Exists(Path) -> Result<bool>;

    static auto IsDirectory(Path) -> Result<bool>;

    static auto CopyFile(Path, Path) -> Result<void>;

    static auto FileSize(Path) -> Result<u64>;

    static auto FileMtime(Path) -> Result<std::filesystem::file_time_type>;

    static auto JsonRead(Path) -> Result<nlohmann::json>;

    static auto JsonWrite(Path) -> Result<void>;

    static auto SafeResolve(Path root, StringView rel) -> Result<Path>;
};
}  // namespace launcher
