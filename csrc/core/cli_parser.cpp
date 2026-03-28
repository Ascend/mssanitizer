/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */


#include "cli_parser.h"

#include <getopt.h>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <iterator>
#include <algorithm>
#include <cstdio>
#include <dlfcn.h>

#include "core/framework/config.h"
#include "core/framework/utility/file_system.h"
#include "core/framework/utility/path.h"
#include "core/framework/utility/ustring.h"
#include "framework/command.h"
#include "framework/config.h"
#include "securec.h"

namespace {
using namespace Sanitizer;
constexpr mode_t DEFAULT_UMASK_FOR_LOG_FILE = 0177;

// 新增参数要考虑用户输入限制，如是否允许数字或特殊字符，在isInWhiteList中更新规则
enum class OptVal : int32_t {
    LOG_LEVEL = 0,
    LOG_FILE,
    LEAK_CHECK,
    CHECK_DEVICE_HEAP,
    CHECK_CANN_HEAP,
    CHECK_UNUSED_MEM,
    MAX_DEBUGLOG_SIZE,
    BLOCK_ID,
    CACHE_SIZE,
    FULL_BACKTRACE,
    KERNEL_NAME,
    DEMANGLE_MODE,
};

std::vector<option> GetLongOptArray()
{
    std::vector<option> longOpts = {
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {"tool", required_argument, nullptr, 't'},
        {"log-level", required_argument, nullptr, static_cast<int32_t>(OptVal::LOG_LEVEL)},
        {"log-file", required_argument, nullptr, static_cast<int32_t>(OptVal::LOG_FILE)},
        {"max-debuglog-size", required_argument, nullptr, static_cast<int32_t>(OptVal::MAX_DEBUGLOG_SIZE)},
        {"leak-check", required_argument, nullptr, static_cast<int32_t>(OptVal::LEAK_CHECK)},
        {"check-unused-memory", required_argument, nullptr, static_cast<int32_t>(OptVal::CHECK_UNUSED_MEM)},
        {"check-device-heap", required_argument, nullptr, static_cast<int32_t>(OptVal::CHECK_DEVICE_HEAP)},
        {"check-cann-heap", required_argument, nullptr, static_cast<int32_t>(OptVal::CHECK_CANN_HEAP)},
        {"block-id", required_argument, nullptr, static_cast<int32_t>(OptVal::BLOCK_ID)},
        {"cache-size", required_argument, nullptr, static_cast<int32_t>(OptVal::CACHE_SIZE)},
        {"kernel-name", required_argument, nullptr, static_cast<int32_t>(OptVal::KERNEL_NAME)},
        {"full-backtrace", required_argument, nullptr, static_cast<int32_t>(OptVal::FULL_BACKTRACE)},
        {"demangle", required_argument, nullptr, static_cast<int32_t>(OptVal::DEMANGLE_MODE)},
        {nullptr, 0, nullptr, 0},
    };
    return longOpts;
}

std::string GetShortOptString(const std::vector<option> &longOptArray)
{
    // 根据long option string生成short option string
    std::string shortOpt;
    for (const auto &opt : longOptArray) {
        if (opt.name == nullptr) {
            break;
        }
        if ((opt.flag == nullptr) && (opt.val >= 'a') && (opt.val <= 'z')) {
            shortOpt.append(1, static_cast<char>(opt.val));
            if (opt.has_arg == optional_argument) {
                shortOpt.append(2, ':'); // 可不跟参数使用2个冒号，如 "a::"
            } else if (opt.has_arg == required_argument) {
                shortOpt.append(1, ':'); // 必须紧跟参数使用1个冒号，如 "t:"
            } else {
                // do nothing
            }
        }
    }
    return shortOpt;
}

void ParseTool(const std::string &param, UserCommand &userCommand)
{
    if (param == "memcheck") {
        userCommand.config.memCheck = true;
        userCommand.config.defaultCheck = true;
        userCommand.config.registerCheck = true;
        SAN_BUFF_INFO_LOG("Set mode param: memcheck");
    } else if (param == "racecheck") {
        userCommand.config.raceCheck = true;
        SAN_BUFF_INFO_LOG("Set mode param: racecheck");
    } else if (param == "initcheck") {
        userCommand.config.initCheck = true;
        userCommand.config.defaultCheck = true;
        SAN_BUFF_INFO_LOG("Set mode param: initcheck");
    } else if (param == "synccheck") {
        userCommand.config.syncCheck = true;
        SAN_BUFF_INFO_LOG("Set mode param: synccheck");
    } else {
        std::cout << "[mssanitizer] ERROR: --tool param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    }
}

void ParseLogLv(const std::string &param, UserCommand &userCommand)
{
    static const std::map<std::string, LogLv> LOG_LV_LOOKUP_TABLE = {
        {"info", LogLv::INFO},
        {"warn", LogLv::WARN},
        {"error", LogLv::ERROR},
    };
    auto it = LOG_LV_LOOKUP_TABLE.find(param);
    if (it == LOG_LV_LOOKUP_TABLE.end()) {
        std::cout << "[mssanitizer] ERROR: --log-level param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    } else {
        userCommand.logLv = it->second;
        SAN_BUFF_INFO_LOG("Set log-level param: %s", it->first.c_str());
    }
}
bool IsLogFileSafe(const std::string &filepath)
{
    if (filepath.empty()) {
        return false;
    }
    auto logger = [](std::string const &msg) { std::cout <<"[mssanitizer] " << msg << std::endl; };
    if (!IsSafeLogFile(filepath, logger)) {
        std::cout << "[mssanitizer] ERROR: The assigned file is not safe for log. Please check the permission or try "
                     "another file."
                  << std::endl;
        return false;
    }
    UmaskGuard guard{DEFAULT_UMASK_FOR_LOG_FILE};
    std::ofstream fd(filepath, std::ofstream::out | std::ofstream::trunc);
    if (!fd.is_open()) {
        std::cout << "[ERROR] Failed to open the log file. Please check the path or try another one." << std::endl;
        std::string curPathStr = Utility::ReplaceInvalidChar(filepath);
        SAN_BUFF_ERROR_LOG("Failed to set log-file: %s. Unable to the truncate log file.", curPathStr.c_str());
        return false;
    }
    return true;
}

void ParseLogFile(const std::string &param, UserCommand &userCommand)
{
    // 日志名长度校验：4096B
    if (param.size() > MAX_FILE_PATH_LEN) {
        std::cout << "[mssanitizer] WARNING: --log-file is invalid because it exceeds the maximum path length of "
                  << std::to_string(MAX_FILE_PATH_LEN) << ". Stdout will be used for logging instead."
                  << std::endl;
        return;
    }
    userCommand.logFileAvailable = false;
    if (param.empty()) {
        std::cout << "[ERROR] Empty log file." << std::endl;
        return;
    }
    Path logPath(param);
    auto logPathStr = Path(param).Resolved().ToString();
    if (!IsLogFileSafe(logPathStr)) {
        // 会导致在拉起用户程序前退出
        return;
    }
    userCommand.logFileAvailable = true;
    userCommand.logFile = logPathStr;
    std::string curPathStr = Utility::ReplaceInvalidChar(logPathStr);
    SAN_BUFF_INFO_LOG("Set log-file: %s", curPathStr.c_str());
}

void ParseBlockId(const std::string &param, UserCommand &userCommand)
{
    constexpr uint16_t maxBlockId = 200L;
    auto parseFailed = [&userCommand](void) {
        std::cout << "[mssanitizer] ERROR: --block-id param is invalid. "
                  << "range: [0, " << maxBlockId << "]" << std::endl;
        userCommand.printHelpInfo = true;
    };

    long blockId{};
    try {
        blockId = std::stol(param);
    } catch (std::exception& e) {
        return parseFailed();
    }

    if (blockId < 0 || blockId > maxBlockId) {
        return parseFailed();
    }
    userCommand.config.checkBlockId = blockId;
}

void ParseCacheSize(const std::string &param, UserCommand &userCommand)
{
    auto parseFailed = [&userCommand](void) {
        std::cout << "[mssanitizer] ERROR: --cache-size param is invalid. "
                  << "range: [1, " << MAX_RECORD_BUF_SIZE_EACH_BLOCK  << "](MB)" << std::endl;
        userCommand.printHelpInfo = true;
    };

    uint32_t cacheSize{};
    try {
        cacheSize = std::stoul(param);
    } catch (std::exception& e) {
        return parseFailed();
    }

    if (cacheSize == 0 || cacheSize > MAX_RECORD_BUF_SIZE_EACH_BLOCK) {
        return parseFailed();
    }
    userCommand.config.cacheSize = cacheSize;
}

void ParseMaxDebugLogSize(const std::string &param, UserCommand &userCommand)
{
    constexpr long maxRate = 10240L;
    auto parseFailed = [&userCommand](void) {
        std::cout << "[mssanitizer] ERROR: --max-debuglog-size param is invalid. "
                  << "range: 1-" << maxRate << " (MB), default:1024 (MB)" << std::endl;
        userCommand.printHelpInfo = true;
    };

    long rate{};
    try {
        rate = std::stol(param);
    } catch (std::exception& e) {
        return parseFailed();
    }

    if (rate <= 0 || rate > maxRate) {
        return parseFailed();
    }

    Sanitizer::Log& log = *Sanitizer::Log::GetLog();
    log.SetMaxDebugLogSizeRate(rate);
}

void SetPluginPath(UserCommand &userCommand)
{
    char buffer[PATH_MAX] = {};
    int ret = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (ret == -1) {
        std::string message = "[mssanitizer] WARN: internal error: get empty plugin path,"
            " dynamic binary instruction will be disabled";
        std::cout << message << std::endl;
        return;
    }
    Path toolPath{buffer};
    Path pluginPath = toolPath.Parent().Parent() / Path("lib64/libsanplugin_boundscheck.so");
    auto pluginPathStr = pluginPath.Resolved().ToString();
    if (pluginPathStr.length() >= sizeof(userCommand.config.pluginPath)) {
        std::string message = "[mssanitizer] WARN: the CANN path is too long, dbi will be disabled."
            " Please move the CANN to short path which length < 200 to enable dbi";
        std::cout << message << std::endl;
        return;
    }
    pluginPathStr.copy(userCommand.config.pluginPath, sizeof(userCommand.config.pluginPath) - 1);
}

std::string GetRandomDumpPath()
{
    char buf[DUMP_PATH_MAX];
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    size_t len = std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&time));
    std::string tempDumpDir = ".mssanitizer_tmp_" + std::string{buf, len};
    return tempDumpDir;
}

void ParseUnrecognized(const std::string &param, UserCommand &userCommand)
{
    std::cout << "[mssanitizer] ERROR: unrecognized command " << std::endl;
    userCommand.printHelpInfo = true;
}

void ParseHelp(const std::string &param, UserCommand &userCommand)
{
    userCommand.printHelpInfo = true;
}

void ParseVersion(const std::string &param, UserCommand &userCommand)
{
    userCommand.printVersionInfo = true;
}

void ParseFullBacktrace(const std::string &param, UserCommand &userCommand)
{
    if (param == "yes") {
        userCommand.config.isPrintFullStack = true;
        SAN_BUFF_INFO_LOG("Set full-backtrace on");
    } else if (param == "no") {
        userCommand.config.isPrintFullStack = false;
    } else {
        std::cout << "[mssanitizer] ERROR: --full-backtrace param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    }
}

void ParseLeakCheckMode(const std::string &param, UserCommand &userCommand)
{
    if (param == "yes") {
        userCommand.config.leakCheck = true;
        SAN_BUFF_INFO_LOG("Set leak-check on");
    } else if (param == "no") {
        userCommand.config.leakCheck = false;
    } else {
        std::cout << "[mssanitizer] ERROR: --leak-check param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    }
}

void ParseCheckDeviceHeap(const std::string &param, UserCommand &userCommand)
{
    if (param == "yes") {
        userCommand.config.checkDeviceHeap = true;
        userCommand.config.leakCheck = true;
        SAN_BUFF_INFO_LOG("Set check-device-heap & leak-check on");
    } else if (param == "no") {
    } else {
        std::cout << "[mssanitizer] ERROR: --check-device-heap param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    }
}

void ParseCheckCannHeap(const std::string &param, UserCommand &userCommand)
{
    if (param == "yes") {
        userCommand.config.checkCannHeap = true;
        userCommand.config.leakCheck = true;
        SAN_BUFF_INFO_LOG("Set check-cann-heap & leak-check on");
    } else if (param == "no") {
    } else {
        std::cout << "[mssanitizer] ERROR: --check-cann-heap param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
    }
}

void ParseCheckUnusedMemory(const std::string &param, UserCommand &userCommand)
{
    if (param == "yes") {
        userCommand.config.checkUnusedMemory = true;
        SAN_BUFF_INFO_LOG("Set check-unused-memory on");
    } else if (param == "no") {
    } else {
        std::cout << "[mssanitizer] ERROR: --check-unused-memory param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
        return;
    }
}

void ParseKernelName(const std::string &param, UserCommand &userCommand)
{
    if (param.empty()) {
        return;
    }
    if (param.length() > KERNEL_NAME_MAX - 1) {
        std::cout << "[mssanitizer] ERROR: --kernel-name param is invalid. "
                  << "require length <= " << KERNEL_NAME_MAX - 1 << std::endl;
        userCommand.printHelpInfo = true;
        return;
    }
    param.copy(userCommand.config.kernelName, KERNEL_NAME_MAX - 1);
}

void ParseDemangleMode(const std::string &param, UserCommand &userCommand)
{
    if (param == "full") {
        userCommand.config.demangleMode = DemangleMode::FULL_DEMANGLED_NAME;
        SAN_BUFF_INFO_LOG("Set full demangle mode");
    } else if (param == "simple") {
        userCommand.config.demangleMode = DemangleMode::SIMPLE_DEMANGLED_NAME;
        SAN_BUFF_INFO_LOG("Set simple demangle mode");
    } else if (param == "no") {
        SAN_BUFF_INFO_LOG("Set no demangle mode");
        userCommand.config.demangleMode = DemangleMode::MANGLED_NAME;
    } else {
        std::cout << "[mssanitizer] ERROR: --demangle param is invalid" << std::endl;
        userCommand.printHelpInfo = true;
        return;
    }
}

using ParseHandler = std::function<void(const std::string &, UserCommand &)>;
std::unordered_map<int32_t, ParseHandler>& GetCommandHandlers()
{
    static std::unordered_map<int32_t, ParseHandler> handlers = {
        {'?', ParseUnrecognized},
        {'h', ParseHelp},
        {'v', ParseVersion},
        {'t', ParseTool},
        {static_cast<int32_t>(OptVal::CHECK_DEVICE_HEAP), ParseCheckDeviceHeap},
        {static_cast<int32_t>(OptVal::CHECK_CANN_HEAP), ParseCheckCannHeap},
        {static_cast<int32_t>(OptVal::LOG_LEVEL), ParseLogLv},
        {static_cast<int32_t>(OptVal::LOG_FILE), ParseLogFile},
        {static_cast<int32_t>(OptVal::LEAK_CHECK), ParseLeakCheckMode},
        {static_cast<int32_t>(OptVal::CHECK_UNUSED_MEM), ParseCheckUnusedMemory},
        {static_cast<int32_t>(OptVal::MAX_DEBUGLOG_SIZE), ParseMaxDebugLogSize},
        {static_cast<int32_t>(OptVal::BLOCK_ID), ParseBlockId},
        {static_cast<int32_t>(OptVal::CACHE_SIZE), ParseCacheSize},
        {static_cast<int32_t>(OptVal::KERNEL_NAME), ParseKernelName},
        {static_cast<int32_t>(OptVal::FULL_BACKTRACE), ParseFullBacktrace},
        {static_cast<int32_t>(OptVal::DEMANGLE_MODE), ParseDemangleMode},
    };

    return handlers;
}


void ParseUserCommand(const int32_t &opt, const std::string &param, UserCommand &userCommand)
{
    auto handlers = GetCommandHandlers();
    auto iter = handlers.find(opt);
    if (iter != handlers.end()) {
        iter->second(param, userCommand);
    }
}

void ShowDescription()
{
    std::cout <<
        "mssanitizer(MindStudio Sanitizer) is part of MindStudio Operator-dev Tools." << std::endl <<
        "mssanitizer is a functional correctness checking suite. This suite contains" << std::endl <<
        "multiple tools that can perform different type of checks. The features contain" << std::endl <<
        "memcheck, racecheck, initcheck and synccheck now." << std::endl;
}

void ShowHelpInfo()
{
    ShowDescription();
    std::cout <<
        std::endl <<
        "Usage: mssanitizer <option(s)> prog-and-args" << std::endl <<
        std::endl <<
        "  basic user options, with default in [ ]:" << std::endl <<
        "    -h --help            show this message" << std::endl <<
        "    -v --version         show version" << std::endl <<
        "    -t --tool=<name>     use the asan tool named <name> [memcheck|racecheck|initcheck|synccheck]" << std::endl <<
        "    --full-backtrace     print the full backtrace including Ascend C internal calls."  << std::endl <<
        "    --log-file=<file>    log messages to <file>" << std::endl <<
        "    --log-level=<level>  set log level to <level> [warn]" << std::endl <<
        "    --max-debuglog-size=<size>" << std::endl <<
        "                         set debuglog file's max size to <size> (MB), default:1024" << std::endl <<
        "    --kernel-name=<name> only the kernel with the specified name <kernel> is going to be checked" << std::endl <<
        "    --demangle=<mode>    set the demangling <mode> of device funtion name. [full]" << std::endl <<
        std::endl <<
        "  user options for Memcheck:" << std::endl <<
        "    --leak-check=no|yes  search for memory leaks at exit [no]" << std::endl <<
        "    --check-unused-memory=no|yes" << std::endl <<
        "                         search for unused memory allocations [no]" << std::endl <<
        "    --check-device-heap=no|yes" << std::endl <<
        "                         enable device heap check [no]" << std::endl <<
        "    --check-cann-heap=no|yes" << std::endl <<
        "                         enable cann heap check [no]" << std::endl <<
        "    --block-id=<block-id>" << std::endl <<
        "                         set check block id, default check all block" << std::endl <<
        "    --cache-size=<size>" << std::endl <<
        "                         set single block records size to <size> (MB), default:100, max: " << MAX_RECORD_BUF_SIZE_EACH_BLOCK
        << std::endl;
}

std::string GetFuncInjectionRevision()
{
    std::string revision = "<unknown>";

    Path exePath;
    if (!GetSelfExePath(exePath)) {
        return revision;
    }

    Path injectionPath = exePath.Parent().Parent() / Path("lib64/libmssanitizer_injection.so");
    if (!injectionPath.PathCanonicalize().Exists()) {
        return revision;
    }
    void *handle = dlopen(injectionPath.PathCanonicalize().ToString().c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        return revision;
    }

    using FuncType = char const *(*)();
    FuncType func = reinterpret_cast<FuncType>(dlsym(handle, "GetFuncInjectionRevision"));
    if (func != nullptr) {
        auto ret = func();
        if (ret != nullptr) {
            revision = ret;
        }
    }

    dlclose(handle);
    return revision;
}

void ShowVersion()
{
    ShowDescription();
    std::cout << std::endl <<
        "revision:" << std::endl <<
        "  mssanitizer " << __PACKAGE_VERSION__ << "-" << __MSSANITIZER_COMMIT_REVISION__ << std::endl <<
        "  msopscommon " << GetFuncInjectionRevision() << std::endl;
}

bool FindExeInPath(std::string const &exeName, Path const &dirPath, Path &exePath)
{
    std::vector<Path> filePaths;
    if (!ListDir(dirPath, std::back_inserter(filePaths))) {
        return false;
    }

    for (auto const &fpath : filePaths) {
        if (fpath.Name() != exeName) {
            continue;
        }

        struct stat fileStat{};
        if (!fpath.GetStat(fileStat)) {
            continue;
        }
        if (IsFileExecutable(fileStat)) {
            exePath = fpath;
            return true;
        }
    }

    return false;
}

bool FindExe(std::string const &exeName, Path &exePath)
{
    // 优先从当前路径下查找可执行文件
    if (FindExeInPath(exeName, Path("."), exePath)) {
        return true;
    }

    char *pathEnv = getenv("PATH");
    if (pathEnv == nullptr || std::string(pathEnv).empty()) {
        return false;
    }

    // 遍历 PATH 环境变量中的路径，并遍历每个路径下的可执行文件，如果存在
    // 可执行文件的文件名与用户输入的 exeName 一致则返回此文件路径
    std::vector<std::string> paths;
    Utility::Split(std::string(pathEnv), std::back_inserter(paths), ":");
    for (auto const &p : paths) {
        if (FindExeInPath(exeName, Path(p), exePath)) {
            return true;
        }
    }

    return false;
}

bool ResolveExePath(UserCommand &userCommand, std::string &msg)
{
    /// 检查用户命令行参数是否未指定用户程序
    if (userCommand.cmd.empty()) {
        msg = "ERROR: no program specified";
        return false;
    }

    std::string &cmd = userCommand.cmd[0];
    Path exePath;
    if (cmd.find("/") == std::string::npos) {
        if (!FindExe(cmd, exePath)) {
            msg = "ERROR: find executable from PATH environment FAILED";
            return false;
        }
    } else {
        exePath = Path(cmd);
        if (!exePath.Exists()) {
            msg = "ERROR: program path NOT found";
            return false;
        }
    }

    cmd = exePath.Resolved().ToString();
    if (cmd.length() <= MAX_FILE_PATH_LEN) {
        SAN_BUFF_INFO_LOG("Resolved executable path: %s", Utility::ReplaceInvalidChar(cmd).c_str());
    } else {
        SAN_BUFF_WARN_LOG("Executable path exceeds the limit (%u), no record here", MAX_FILE_PATH_LEN);
    }

    return true;
}

bool CheckUserCommand(UserCommand const &userCommand, std::string &msg)
{
    std::string const &cmd = userCommand.cmd[0];

    std::string fmtCmd = Utility::ReplaceInvalidChar(cmd);
    /// 检查用户程序文件状态是否读取成功
    struct stat progStat{};
    if (!Path(fmtCmd).GetStat(progStat)) {
        msg = "ERROR: FAILED to get user program (" + fmtCmd + ") stat";
        return false;
    }
    // 检查用户程序是否具有可执行权限
    if (!IsFileExecutable(progStat)) {
        msg = "ERROR: User program (" + fmtCmd + ") has no execute permission";
        return false;
    }
    if (userCommand.config.checkCannHeap && userCommand.config.checkDeviceHeap) {
        msg = "ERROR: CANNOT enable both --check-cann-heap and --check-device-heap";
        return false;
    }

    return true;
}

void DoUserCommand(const UserCommand &userCommand)
{
    if (userCommand.printHelpInfo) {
        ShowHelpInfo();
        return;
    }
    if (userCommand.printVersionInfo) {
        ShowVersion();
        return;
    }
    if (!userCommand.logFileAvailable) {
        return;
    }

    std::string msg;
    UserCommand resolvedCommand(userCommand);
    if (!ResolveExePath(resolvedCommand, msg) || !CheckUserCommand(resolvedCommand, msg)) {
        std::cout << "[mssanitizer] " << msg << std::endl;
        std::cout << "[mssanitizer] Use --help for more information" << std::endl;
        return;
    }
    Command cmd(resolvedCommand.config, resolvedCommand.logLv, resolvedCommand.logFile);
    cmd.Exec(resolvedCommand.cmd);
    DetectDumpProject(cmd, resolvedCommand.config.dumpPath);
}
}

namespace Sanitizer {
void CliParser::Interpretor(int32_t argc, char **argv) const
{
    auto userCommand = Parse(argc, argv);
    DoUserCommand(userCommand);
}

// 日志黑名单和长度校验
void CheckLogSecurity(const std::string& userInputStr)
{
    // 日志黑名单校验：是否包含\r、\b、\n、\f、\x08、\u0008、\u007F、\x1b[D等字符
    static const std::array<std::string, 10> logBlackList = {"\r", "\b",
        "\n", "\f", "\t", "\v", "\x08", "\u0008", "\u007F", "\x1b[D"};
    bool isInBlackList = false;
    for (auto &item : logBlackList) {
        if (userInputStr.find(item) != std::string::npos) {
            isInBlackList = true;
            break;
        }
    }
    if (userInputStr.length() <= MAX_FILE_PATH_LEN && !isInBlackList) {
        SAN_BUFF_INFO_LOG("Non-option argument: %s", userInputStr.c_str());
    } else {
        SAN_BUFF_WARN_LOG(
            "This non-option argument exceeds the length limit (%u) or contains invalid characters, no record here",
            MAX_FILE_PATH_LEN);
    }
}

inline bool IsLowerAlphabet(char ch)
{
    return (ch >= 'a' && ch <= 'z');
}

inline bool IsUpperAlphabet(char ch)
{
    return (ch >= 'A' && ch <= 'Z');
}

inline bool IsDigital(char ch)
{
    return (ch >= '0' && ch <= '9');
}

inline bool IsSymbolic(char ch)
{
    return (ch == '-' || ch == '.' || ch == '/' || ch == '_');
}

bool IsSatisfyKernelName(int32_t opt, char ch)
{
    if (opt == static_cast<int32_t>(OptVal::KERNEL_NAME)) {
        if (IsLowerAlphabet(ch) || IsUpperAlphabet(ch) || IsDigital(ch) ||
            (ch == '_') || (ch == '-')) {
            return true;
        }
    }
    return false;
}

bool IsSupportDigital(int32_t opt, char ch)
{
    if ((opt == static_cast<int32_t>(OptVal::LOG_FILE) ||
        opt == static_cast<int32_t>(OptVal::MAX_DEBUGLOG_SIZE) ||
        opt == static_cast<int32_t>(OptVal::BLOCK_ID) ||
        opt == static_cast<int32_t>(OptVal::CACHE_SIZE)) &&
        IsDigital(ch)) {
        return true;
    }
    return false;
}

// 参数白名单校验（针对带key的参数，如-t；如果直接输入t，会被视为non-option argument）
bool IsInWhiteList(std::string& param, int32_t opt)
{
    // 入参help/version不带值，param为空，不会进入这个判断；其他参数帯值，分3种要求过滤：
    // 1.log-file只允许数字、字母和[- . / _]，2.max-debuglog-size只允许数字，3.其他只允许小写字母
    for (char ch : param) {
        // 小写字母除了max-debuglog-size，其他所有入参都允许通过
        if (opt != static_cast<int32_t>(OptVal::MAX_DEBUGLOG_SIZE) &&
            IsLowerAlphabet(ch)) {
            continue;
        }
        // 入参log-file和max-debuglog-size允许数字
        if (IsSupportDigital(opt, ch)) {
            continue;
        }
        // 入参log-file额外允许大写字母和符号[- . / _]
        if (opt == static_cast<int32_t>(OptVal::LOG_FILE) && (IsUpperAlphabet(ch) ||
            IsSymbolic(ch))) {
            continue;
        }
        if (IsSatisfyKernelName(opt, ch)) {
            continue;
        }
        return false;
    }
    return true;
}

UserCommand CliParser::Parse(int32_t argc, char **argv) const
{
    UserCommand userCommand;
    SetPluginPath(userCommand);
    auto dumpPath = GetRandomDumpPath();
    dumpPath.copy(userCommand.config.dumpPath, dumpPath.size());

    int32_t optionIndex = 0;
    int32_t opt = 0;
    auto longOptions = GetLongOptArray();
    std::string shortOptions = GetShortOptString(longOptions);
    optind = 0;
    while ((opt = getopt_long(argc, argv, shortOptions.c_str(), longOptions.data(),
        &optionIndex)) != -1) {
        // somehow optionIndex is not always correct for short option.
        // match it on our own.
        for (uint32_t i = 0; i < longOptions.size(); ++i) {
            if (longOptions[i].val == opt) {
                optionIndex = static_cast<int32_t>(i);
                break;
            }
        }
        std::string param;
        if (optarg) {
            param = optarg;
        }
        // 不在白名单内的参数直接跳过，不解析
        if (!IsInWhiteList(param, opt)) {
            userCommand.printHelpInfo = true;
            std::cout << "[mssanitizer] ERROR: param '" << longOptions[optionIndex].name
                      << "' contains invalid characters and is ignored.\n" << std::endl;
            return userCommand;
        }
        ParseUserCommand(opt, param, userCommand);
        // 需打印help或version亦或日志文件校验不通过时，不进行其他操作
        if (userCommand.printHelpInfo || userCommand.printVersionInfo || !userCommand.logFileAvailable) {
            return userCommand;
        }
    }

    /// 如果defaultCheck为false，有3种情况：
    /// (1) -t只开启raceCheck, (2) -t没开initCheck或memCheck，但是开了内存检测附加功能, (3) 命令行没有参数
    /// 后2种情况需要开启defaultCheck和memCheck
    if (userCommand.config.leakCheck || userCommand.config.checkUnusedMemory ||
        (!userCommand.config.defaultCheck && !userCommand.config.raceCheck && !userCommand.config.syncCheck)) {
        userCommand.config.defaultCheck = true;
        userCommand.config.memCheck = true;
        userCommand.config.registerCheck = true;
        SAN_BUFF_INFO_LOG("Set mode param: memcheck");
    }

    std::vector<std::string> userBinCmd;
    for (; optind < argc; optind++) {
        // 对写入日志的算子可执行文件(non-option argument)设置黑名单，只针对日志，不影响执行
        CheckLogSecurity(argv[optind]);
        userBinCmd.emplace_back(argv[optind]);
    }
    userCommand.cmd = userBinCmd;
    return userCommand;
}

}
