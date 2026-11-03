// Checks claims made by docs/security/deployment.md.
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#if !defined(_WIN32) && __has_include(<grp.h>) && __has_include(<unistd.h>)
#  include <grp.h>
#  include <sys/resource.h>
#  include <sys/types.h>
#  include <unistd.h>
#  define CPPDOCS_POSIX_PRIVILEGES 1  // NOLINT(cppcoreguidelines-macro-usage): tested by #if
#endif
#if defined(__linux__)
#  include <sys/mman.h>
#  include <sys/prctl.h>
#endif

#include <gtest/gtest.h>

namespace {

class Secret {
public:
    explicit Secret(std::string value) : value_{std::move(value)} {}
    const std::string& reveal() const { return value_; }
private:
    std::string value_;
};

std::ostream& operator<<(std::ostream& os, const Secret&) { return os << "[redacted]"; }

std::string log_field(std::string_view untrusted) {
    std::string out;
    for (unsigned char c : untrusted) {
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c < 0x20 || c == 0x7f) out += std::format("\\x{:02x}", c);
        else out += static_cast<char>(c);
    }
    return out;
}

bool write_atomically(const std::filesystem::path& target, std::string_view data) {
    std::filesystem::path tmp = target;
    tmp += ".tmp";
    {
        std::ofstream out{tmp, std::ios::binary | std::ios::trunc};
        if (!out.write(data.data(), static_cast<std::streamsize>(data.size()))) return false;
        out.flush();
        if (!out) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, target, ec);
    return !ec;
}

std::string slurp(const std::filesystem::path& p) {
    std::ifstream in{p, std::ios::binary};
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

#if defined(CPPDOCS_POSIX_PRIVILEGES)
void drop_privileges(uid_t uid, gid_t gid) {
    if (setgroups(0, nullptr) != 0) throw std::runtime_error("setgroups failed");
    if (setgid(gid) != 0) throw std::runtime_error("setgid failed");
    if (setuid(uid) != 0) throw std::runtime_error("setuid failed");
    if (uid != 0 && setuid(0) == 0) throw std::runtime_error("root could be regained");
}
#endif

}  // namespace

template <>
struct std::formatter<Secret> : std::formatter<std::string_view> {
    auto format(const Secret&, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format("[redacted]", ctx);
    }
};

// "Keep logs safe to keep": a Secret prints as [redacted] through std::format and
// streams; its value is reachable only through reveal().
TEST(SecurityDeployment, SecretNeverPrints) {
    const Secret token{"hunter2"};
    EXPECT_EQ(std::format("token={}", token), "token=[redacted]");
    std::ostringstream os;
    os << token;
    EXPECT_EQ(os.str(), "[redacted]");
    EXPECT_EQ(token.reveal(), "hunter2");
}

// "Neutralize untrusted text": a newline can't start a forged log entry, and
// terminal escapes become visible.
TEST(SecurityDeployment, LogFieldIsOneLine) {
    const std::string forged = log_field("alice\nINFO admin logged in");
    EXPECT_EQ(forged, "alice\\nINFO admin logged in");
    EXPECT_EQ(forged.find('\n'), std::string::npos);
    EXPECT_EQ(log_field("\x1b[2J"), "\\x1b[2J");
    EXPECT_EQ(log_field("caf\xc3\xa9"), "caf\xc3\xa9");   // UTF-8 passes through
}

// "Run with least privilege": an unprivileged process can't drop to another
// identity, and drop_privileges reports that instead of carrying on.
TEST(SecurityDeployment, FailedDropThrows) {
#if defined(CPPDOCS_POSIX_PRIVILEGES)
    if (geteuid() == 0) GTEST_SKIP() << "running as root: the drop would succeed and change this process";
    EXPECT_THROW(drop_privileges(getuid(), getgid()), std::runtime_error);   // setgroups needs privilege
#else
    GTEST_SKIP() << "POSIX privileges only (no <grp.h>/<unistd.h>)";
#endif
}

// "Keep secrets out of crash dumps": the core-file limit can be lowered to 0 by
// an unprivileged process.
TEST(SecurityDeployment, CoreLimitCanBeLowered) {
#if defined(CPPDOCS_POSIX_PRIVILEGES)
    rlimit saved{};
    ASSERT_EQ(getrlimit(RLIMIT_CORE, &saved), 0);
    rlimit none = saved;
    none.rlim_cur = 0;   // soft limit only, so it can be restored
    ASSERT_EQ(setrlimit(RLIMIT_CORE, &none), 0);
    rlimit now{};
    ASSERT_EQ(getrlimit(RLIMIT_CORE, &now), 0);
    EXPECT_EQ(now.rlim_cur, rlim_t{0});
    EXPECT_EQ(setrlimit(RLIMIT_CORE, &saved), 0);
#else
    GTEST_SKIP() << "POSIX resource limits only";
#endif
}

// "Keep secrets out of crash dumps": on Linux a process can mark itself not
// dumpable and exclude pages from dumps.
TEST(SecurityDeployment, LinuxNotDumpableAndDontDump) {
#if defined(__linux__)
    ASSERT_EQ(prctl(PR_SET_DUMPABLE, 0, 0, 0, 0), 0);
    EXPECT_EQ(prctl(PR_GET_DUMPABLE, 0, 0, 0, 0), 0);
    EXPECT_EQ(prctl(PR_SET_DUMPABLE, 1, 0, 0, 0), 0);   // restore: sanitizers' leak checks attach at exit
    const long page = sysconf(_SC_PAGESIZE);
    void* p = mmap(nullptr, static_cast<std::size_t>(page), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(p, MAP_FAILED);
    EXPECT_EQ(madvise(p, static_cast<std::size_t>(page), MADV_DONTDUMP), 0);
    munmap(p, static_cast<std::size_t>(page));
#else
    GTEST_SKIP() << "Linux only (prctl, MADV_DONTDUMP)";
#endif
}

// "Isolate the code that parses untrusted input": setting no_new_privs works
// unprivileged, and can't be unset.
TEST(SecurityDeployment, LinuxNoNewPrivs) {
#if defined(__linux__)
    // Run in a child so the test process keeps its ability to gain privileges.
    EXPECT_EXIT({
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) std::_Exit(1);
        if (prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) != 1) std::_Exit(2);
        if (prctl(PR_SET_NO_NEW_PRIVS, 0, 0, 0, 0) == 0) std::_Exit(3);   // one-way
        std::_Exit(0);
    }, testing::ExitedWithCode(0), "");
#else
    GTEST_SKIP() << "Linux only (PR_SET_NO_NEW_PRIVS)";
#endif
}

// "Stop safely, restart cleanly": writing to a temporary file and renaming it
// replaces the target in one step, and leaves no temporary file behind.
TEST(SecurityDeployment, AtomicWriteReplacesTarget) {
    const auto dir = std::filesystem::temp_directory_path() / "cppdocs-deployment-test";
    std::filesystem::create_directories(dir);
    const auto target = dir / "state.txt";
    ASSERT_TRUE(write_atomically(target, "version 1"));
    EXPECT_EQ(slurp(target), "version 1");
    ASSERT_TRUE(write_atomically(target, "version 2"));
    EXPECT_EQ(slurp(target), "version 2");
    EXPECT_FALSE(std::filesystem::exists(dir / "state.txt.tmp"));
    std::filesystem::remove_all(dir);
}
