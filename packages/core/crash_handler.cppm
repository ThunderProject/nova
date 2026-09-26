module;

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cxxabi.h>
#include <dirent.h>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <stacktrace>
#include <string>
#include <string_view>
#include <type_traits>
#include <ucontext.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

export module core.crash_handler;

namespace nova::crash {

    export enum class install_errc : std::uint8_t {
        already_installed,
        must_install_before_threads,
        alt_stack_allocation_failed,
        alt_stack_guard_failed,
        sigaltstack_failed,
        socketpair_failed,
        fork_failed,
        ptracer_configuration_failed,
        sigaction_failed,
    };

    export struct install_error {
        install_errc code{};
        int system_error{};
    };

    export using install_result = std::expected<void, install_error>;

    constexpr std::size_t alternate_stack_size = 256uz * 1024uz;
    constexpr std::size_t max_stack_frames = 256uz;
    constexpr std::array fatal_signals { 
        SIGSEGV, 
        SIGBUS, 
        SIGILL, 
        SIGFPE, 
        SIGABRT,
        SIGSYS
    };

    struct crash_packet {
        std::int32_t signal_number{};
        std::int32_t signal_code{};

        pid_t pid{};
        pid_t tid{};

        std::uintptr_t fault_address{};
        std::uintptr_t instruction_pointer{};
    };

    static_assert(std::is_trivially_copyable_v<crash_packet>);
    static_assert(std::atomic<int>::is_always_lock_free, "Crash handler requires lock-free int atomics");

    std::atomic_flag crash_claimed = ATOMIC_FLAG_INIT;
    std::atomic<int> crash_channel{-1};
    std::atomic<int> crash_output{STDERR_FILENO};

    std::atomic<bool> process_installed{false};

    struct alternate_stack {
        void* mapping{};
        std::size_t mapping_size{};

        stack_t previous{};
        bool installed{};

        alternate_stack() = default;

        alternate_stack(const alternate_stack&) = delete;
        alternate_stack& operator=(const alternate_stack&) = delete;

        ~alternate_stack() {
            if(!installed) {
                return;
            }

            ::sigaltstack(&previous, nullptr);

            if(mapping != nullptr) {
                ::munmap(mapping, mapping_size);
            }
        }
    };

    thread_local alternate_stack thread_alt_stack;

    [[nodiscard]] bool is_decimal_name(const char* name) noexcept {
        if(name == nullptr || *name == '\0') {
            return false;
        }

        for(const char* ptr = name; *ptr != '\0'; ++ptr) {
            if(*ptr < '0' || *ptr > '9') {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool process_is_single_threaded() noexcept {
        auto* directory = ::opendir("/proc/self/task");

        if(directory == nullptr) {
            return false;
        }

        std::size_t thread_count = 0;

        while(auto* entry = ::readdir(directory)) {
            if(!is_decimal_name(entry->d_name)) {
                continue;
            }

            ++thread_count;

            if(thread_count > 1) {
                break;
            }
        }

        ::closedir(directory);

        return thread_count == 1;
    }

    [[nodiscard]] pid_t raw_gettid() noexcept {
        long result{};

        asm volatile(
            "syscall"
            : "=a"(result)
            : "a"(static_cast<long>(SYS_gettid))
            : "rcx", "r11", "memory"
        );

        return static_cast<pid_t>(result);
    }

    [[nodiscard]] std::uintptr_t instruction_pointer(void* context) noexcept {
        if(context == nullptr) {
            return 0;
        }

        const auto* ucontext = static_cast<ucontext_t*>(context);
        return static_cast<std::uintptr_t>(ucontext->uc_mcontext.gregs[REG_RIP]);
    }

    void emergency_write(std::string_view text) noexcept {
        const auto fd = crash_output.load(std::memory_order_relaxed);

        const char* data = text.data();
        std::size_t remaining = text.size();

        while(remaining != 0) {
            const auto result = ::write(fd, data, remaining);

            if(result > 0) {
                data += result;

                remaining -= static_cast<std::size_t>(result);
                continue;
            }

            if(result < 0 && errno == EINTR) {
                continue;
            }

            break;
        }
    }

    [[nodiscard]] constexpr std::string_view signal_name(int signal) noexcept {
        switch(signal) {
            case SIGSEGV: return "SIGSEGV";
            case SIGBUS: return "SIGBUS";
            case SIGILL: return "SIGILL";
            case SIGFPE: return "SIGFPE";
            case SIGABRT: return "SIGABRT";
            case SIGSYS: return "SIGSYS";
            default: return "UNKNOWN";
        }
    }

    [[nodiscard]]std::string demangle(const char* symbol) {
        if(symbol == nullptr) {
            return "<unknown>";
        }

        int status = 0;

        std::unique_ptr<char, decltype(&std::free)> demangled {
            abi::__cxa_demangle(symbol, nullptr, nullptr, &status), &std::free
        };

        if(status == 0 && demangled) {
            return demangled.get();
        }

        return symbol;
    }

    void write_remote_stacktrace(int output_fd, pid_t tid) {
        const auto ptrace_seize = ::ptrace(
            PTRACE_SEIZE, 
            tid, 
            nullptr,
            static_cast<unsigned long>(PTRACE_O_EXITKILL)
        );

        if(ptrace_seize == -1) {
            const auto message = std::format("stacktrace: ptrace seize failed: errno={}\n", errno);
            emergency_write(message);
            return;
        }

        const auto detach = [tid] noexcept {
            ::ptrace(PTRACE_DETACH, tid, nullptr, nullptr);
        };

        if(::ptrace(PTRACE_INTERRUPT, tid, nullptr, nullptr) == -1) {
            detach();
            return;
        }

        int status{};

        if(::waitpid(tid, &status, __WALL) == -1) {
            detach();
            return;
        }

        auto* address_space = unw_create_addr_space(&_UPT_accessors, 0);

        if(address_space == nullptr) {
            detach();
            return;
        }

        const auto destroy_address_space = [&] noexcept { unw_destroy_addr_space(address_space); };

        auto* remote = _UPT_create(tid);

        if(remote == nullptr) {
            destroy_address_space();
            detach();
            return;
        }

        unw_cursor_t cursor{};

        if(unw_init_remote(&cursor, address_space, remote) < 0) {
            _UPT_destroy(remote);
            destroy_address_space();
            detach();
            return;
        }

        std::string output;
        output.reserve(static_cast<size_t>(16 * 1024));

        output += "stacktrace:\n";

        for(std::size_t frame = 0; frame < max_stack_frames; ++frame) {
            unw_word_t ip{};
            unw_word_t offset{};

            if(unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
                break;
            }

            std::array<char, 1024> symbol{};

            const auto symbol_result = unw_get_proc_name(&cursor, symbol.data(), symbol.size(), &offset);

            const auto is_signal_frame = unw_is_signal_frame(&cursor) > 0;

            if(symbol_result == 0) {
                output += std::format(
                    "  #{:03}  0x{:016x}  {} + 0x{:x}{}\n",
                    frame,
                    static_cast<std::uint64_t>(ip),
                    demangle(symbol.data()),
                    static_cast<std::uint64_t>(offset),
                    is_signal_frame ? "  <signal frame>" : ""
                );
            } 
            else {
                output += std::format(
                    "  #{:03}  0x{:016x}  <unknown>{}\n",
                    frame,
                    static_cast<std::uint64_t>(ip),
                    is_signal_frame ? "  <signal frame>" : ""
                );
            }

            const auto step = unw_step(&cursor);

            if(step <= 0) {
                break;
            }
        }

        const char* data = output.data();
        std::size_t remaining = output.size();

        while(remaining != 0) {
            const auto written = ::write(output_fd, data, remaining);

            if(written > 0) {
                data += written;
                remaining -= static_cast<std::size_t>(written);
                continue;
            }

            if(written < 0 && errno == EINTR) {
                continue;
            }

            break;
        }

        _UPT_destroy(remote);

        destroy_address_space();
        detach();
    }

    void report_signal(int output_fd, const crash_packet& packet) noexcept {
        try {
            const auto header = std::format(
                "\n"
                "============================================================\n"
                "                    NOVA FATAL ERROR\n"
                "============================================================\n"
                "kind:                fatal signal\n"
                "signal:              {} ({})\n"
                "signal code:         {}\n"
                "pid:                 {}\n"
                "tid:                 {}\n"
                "fault address:       0x{:016x}\n"
                "instruction pointer: 0x{:016x}\n"
                "------------------------------------------------------------\n",
                signal_name(packet.signal_number),
                packet.signal_number,
                packet.signal_code,
                packet.pid,
                packet.tid,
                packet.fault_address,
                packet.instruction_pointer
            );

            const char* data = header.data();
            std::size_t size = header.size();

            while(size != 0) {
                const auto written = ::write(output_fd, data, size);

                if(written > 0) {
                    data += written;
                    size -= static_cast<std::size_t>(written);
                    continue;
                }

                if(written < 0 && errno == EINTR) {
                    continue;
                }

                break;
            }

            write_remote_stacktrace(output_fd, packet.tid);

            constexpr std::string_view footer = "============================================================\n";

            auto _ = ::write(output_fd, footer.data(), footer.size());
        }
        catch(...) {
            emergency_write("\nNOVA FATAL ERROR\n Crash reporter failed while formatting the report.\n");
        }
    }

    [[noreturn]] void reporter_loop(int channel, int output_fd) noexcept {
        for(;;) {
            crash_packet packet{};

            const auto bytes = ::read(channel, &packet, sizeof(packet));

            if(bytes == 0) {
                std::_Exit(EXIT_SUCCESS);
            }

            if(bytes != static_cast<ssize_t>(sizeof(packet))) {
                if(bytes < 0 && errno == EINTR) {
                    continue;
                }
                std::_Exit(EXIT_FAILURE);
            }

            report_signal(output_fd, packet);

            constexpr std::byte acknowledgement{ 0x01 };

            auto _ = ::write(channel, &acknowledgement, sizeof(acknowledgement));
        }
    }

    extern "C" void fatal_signal_handler(int signal, siginfo_t* information, void* context) noexcept {
        if(crash_claimed.test_and_set(std::memory_order_relaxed)) {
            std::_Exit(128 + signal);
        }

        crash_packet packet{
            .signal_number = signal,
            .signal_code = information != nullptr ? information->si_code : 0,
            .pid = ::getpid(),
            .tid = raw_gettid(),
            .fault_address = information != nullptr ? reinterpret_cast<std::uintptr_t>(information->si_addr) : 0,
            .instruction_pointer = instruction_pointer(context),
        };

        const auto channel = crash_channel.load(std::memory_order_relaxed);

        if(channel >= 0) {
            const auto written = ::write(channel, &packet, sizeof(packet));

            if(written == static_cast<ssize_t>(sizeof(packet))) {
                std::byte acknowledgement{};

                auto _ = ::read(channel, &acknowledgement, sizeof(acknowledgement));
                std::_Exit(128 + signal);
            }
        }

        constexpr char fallback[] = "\nNOVA FATAL ERROR: fatal signal; external crash reporter unavailable\n";

        auto _ = ::write(STDERR_FILENO, fallback, sizeof(fallback) - 1);
        std::_Exit(128 + signal);
    }

    [[noreturn]] void terminate_handler() noexcept {
        if(crash_claimed.test_and_set(std::memory_order_relaxed)) {
            std::_Exit(EXIT_FAILURE);
        }

        try {
            std::string output;
            output.reserve(static_cast<size_t>(16 * 1024));

            output +=
                "\n"
                "============================================================\n"
                "                    NOVA FATAL ERROR\n"
                "============================================================\n"
                "kind: std::terminate\n";

            if(const auto exception = std::current_exception(); exception != nullptr) {
                try {
                    std::rethrow_exception(exception);
                }
                catch(const std::exception& error) {
                    output += std::format("exception: {}\n", error.what());
                }
                catch(...) {
                    output += "exception: non-std exception\n";
                }
            }
            else {
                output += "exception: none\n";
            }

            output +=
                "------------------------------------------------------------\n stacktrace:\n";

            const auto trace = std::stacktrace::current(1);

            if(trace.empty()) {
                output += "  <stacktrace unavailable>\n";
            }
            else {
                try {
                    output += std::to_string(trace);
                    output += '\n';
                }
                catch(...) {
                    output += "  <stacktrace formatting failed>\n";
                }
            }

            output += "============================================================\n";
            emergency_write(output);

        }
        catch(...) {
            emergency_write("\nNOVA FATAL ERROR: std::terminate\n Unable to build detailed crash report.\n");
        }
        std::_Exit(EXIT_FAILURE);
    }

    [[nodiscard]] install_result install_alt_stack() noexcept {
        auto& state = thread_alt_stack;

        if(state.installed) {
            return {};
        }

        const auto page_size = ::sysconf(_SC_PAGESIZE);

        if(page_size <= 0) {
            return std::unexpected(
                install_error {
                    .code = install_errc::alt_stack_allocation_failed,
                    .system_error = errno,
                }
            );
        }

        const auto page = static_cast<std::size_t>(page_size);
        const auto total_size = alternate_stack_size + (page * 2);

        void* mapping = ::mmap(
            nullptr, 
            total_size, 
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
            -1,
            0
        );

        if(mapping == MAP_FAILED) {
            return std::unexpected(
                install_error {
                    .code = install_errc::alt_stack_allocation_failed,
                    .system_error = errno,
                }
            );
        }

        auto* base = static_cast<std::byte*>(mapping);

        if(::mprotect(base, page, PROT_NONE) != 0 || ::mprotect(base + page + alternate_stack_size, page, PROT_NONE) != 0) {
            const auto error = errno;

            ::munmap(mapping, total_size);

            return std::unexpected(
                install_error {
                    .code = install_errc::alt_stack_guard_failed,
                    .system_error = error,
                }
            );
        }

        stack_t stack{
            .ss_sp = base + page,
            .ss_flags = 0,
            .ss_size = alternate_stack_size,
        };

        stack_t previous{};

        if(::sigaltstack(&stack, &previous) != 0) {
            const auto error = errno;

            ::munmap(mapping, total_size);

            return std::unexpected(
                install_error {
                    .code = install_errc::sigaltstack_failed,
                    .system_error = error,
                }
            );
        }

        state.mapping = mapping;
        state.mapping_size = total_size;
        state.previous = previous;
        state.installed = true;

        return {};
    }

    /**
    * Install an alternate crash stack for the calling thread.
    *
    * Call once at the beginning of every long lived worker thread.
    */
    export [[nodiscard]] install_result install_for_current_thread() noexcept { return install_alt_stack(); }

    /**
    * Install process wide fatal crash handling.
    *
    * Call this early in main(), before creating any other threads.
    */
    export [[nodiscard]] install_result install(int output_fd = STDERR_FILENO) noexcept {
        bool expected = false;

        if(!process_installed.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            return std::unexpected(
                install_error {
                    .code = install_errc::already_installed,
                }
            );
        }

        const auto fail = [](install_error error) -> install_result {
            process_installed.store(false, std::memory_order_release);
            return std::unexpected(error);
        };

        if(!process_is_single_threaded()) {
            return fail(
                install_error {
                    .code = install_errc::must_install_before_threads,
                }
            );
        }

        if(auto result = install_alt_stack(); !result) {
            process_installed.store(false, std::memory_order_release);
            return result;
        }

        int sockets[2]{};

        if(::socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) != 0) {
            return fail(
                install_error {
                    .code = install_errc::socketpair_failed,
                    .system_error = errno,
                }
            );
        }

        const auto reporter = ::fork();

        if(reporter < 0) {
            const auto error = errno;

            ::close(sockets[0]);
            ::close(sockets[1]);

            return fail(
                install_error {
                    .code = install_errc::fork_failed,
                    .system_error = error,
                }
            );
        }

        if(reporter == 0) {
            ::close(sockets[0]);
            reporter_loop(sockets[1], output_fd);
        }

        ::close(sockets[1]);

        if(::prctl(PR_SET_PTRACER, reporter, 0, 0, 0) != 0) {
            const auto error = errno;
            ::close(sockets[0]);
            ::kill(reporter, SIGTERM);

            return fail(
                install_error {
                    .code = install_errc::ptracer_configuration_failed,
                    .system_error = error,
                }
            );
        }

        crash_output.store(output_fd, std::memory_order_release);
        crash_channel.store(sockets[0], std::memory_order_release);

        std::set_terminate(terminate_handler);

        struct sigaction action {};
        action.sa_sigaction = fatal_signal_handler;

        action.sa_flags = static_cast<int>(SA_SIGINFO | SA_ONSTACK | SA_RESETHAND);
        ::sigfillset(&action.sa_mask);

        for(const auto signal : fatal_signals) {
            if(::sigaction(signal, &action, nullptr) != 0) {
                return fail(
                    install_error{
                        .code = install_errc::sigaction_failed,
                        .system_error = errno,
                    }
                );
            }
        }
        return {};
    }
}
