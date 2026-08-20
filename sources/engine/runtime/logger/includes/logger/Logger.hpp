#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "core/containers/Buffer.hpp"
#include "core/common/Common.hpp"

using namespace core;
using namespace std::chrono;
namespace runtime
{
static inline std::atomic<unsigned long long> MessageUId = 1;

enum class ELogLevel : uint8_t
{
    Temp,
    Info,
    Warning,
    Error,
};

template <class Derived>
struct Category
{
    constexpr std::string_view getName() const noexcept
    {
        static_assert(
            requires {
                { Derived::CategoryName } -> std::convertible_to<std::string_view>;
            },
            "Derived Category must have a static constexpr CategoryName member convertible to std::string_view");

        return Derived::CategoryName;
    }
    constexpr std::string_view getColor() const noexcept
    {
        static_assert(
            requires {
                { Derived::CategoryColor } -> std::convertible_to<std::string_view>;
            },
            "Derived Category must have a static constexpr CategoryColor member convertible to std::string_view");

        return Derived::CategoryColor;
    }
};

struct DefaultCategory : public Category<DefaultCategory>
{
    static constexpr std::string_view CategoryName = "default";
    static constexpr std::string_view CategoryColor = "0xFFFFFF";
};

#define DECLARE_CATEGORY(NAME, COLOR) \
    struct NAME : public Category<NAME> \
    { \
        static constexpr std::string_view CategoryName = #NAME; \
        static constexpr std::string_view CategoryColor = COLOR; \
    };

class Message
{
public:
    /**
     * @brief 构造一条完整日志消息。
     * @param Id 全局消息唯一 ID。
     * @param InLevel 日志级别。
     * @param Cat 日志分类。
     * @param CalledLoc 调用位置信息。
     * @param Msg 日志文本。
     * @return 构造后的 Message 对象。
     * @usage auto Msg = Message::makeMessage(1, ELogLevel::Info, DefaultCategory{}, std::source_location::current(), "hello");
     */
    template <typename CategoryTy = DefaultCategory>
    static Message makeMessage(
        uint64_t Id,
        ELogLevel InLevel,
        const Category<CategoryTy>& Cat,
        const std::source_location& CalledLoc,
        std::string_view Msg)
    {
        return Message(Id, InLevel, Cat.getName(), CalledLoc, Msg);
    }

    Message() = default;
    ~Message() = default;
    Message(const Message&) = default;
    Message& operator=(const Message&) = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;

    /**
     * @brief 获取日志级别。
     * @return 当前消息的日志级别。
     * @usage auto InLevel = Msg.get_level();
     */
    [[nodiscard]] ELogLevel getLevel() const noexcept { return Level; }
    /**
     * @brief 获取消息 ID。
     * @return 当前消息的全局 ID。
     * @usage auto Id = Msg.get_id();
     */
    [[nodiscard]] uint64_t getId() const noexcept { return Id; }
    /**
     * @brief 获取分类名。
     * @return 分类字符串视图。
     * @usage auto Cat = Msg.getCategoryName();
     */
    [[nodiscard]] std::string_view getCategoryName() const noexcept { return CategoryName; }
    /**
     * @brief 获取调用源文件名。
     * @return 文件名字符串视图。
     * @usage auto file = Msg.getFileName();
     */
    [[nodiscard]] std::string_view getFileName() const noexcept { return FileName; }
    /**
     * @brief 获取调用函数名。
     * @return 函数名字符串视图。
     * @usage auto fn = Msg.getFunctionName();
     */
    [[nodiscard]] std::string_view getFunctionName() const noexcept { return FunctionName; }
    /**
     * @brief 获取调用行号。
     * @return 行号。
     * @usage auto line = Msg.getLine();
     */
    [[nodiscard]] uint_least32_t getLine() const noexcept { return Line; }
    /**
     * @brief 获取调用列号。
     * @return 列号。
     * @usage auto col = Msg.getColumn();
     */
    [[nodiscard]] uint_least32_t getColumn() const noexcept { return Column; }
    /**
     * @brief 获取日志文本。
     * @return 日志文本字符串视图。
     * @usage auto text = Msg.getText();
     */
    [[nodiscard]] std::string_view getText() const noexcept { return MessageText; }
    /**
     * @brief 获取消息时间戳。
     * @return system_clock 时间点。
     * @usage auto ts = Msg.getTimestamp();
     */
    [[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const noexcept { return Timestamp; }

    /**
     * @brief 将消息序列化为一行文本。
     * @param rec 要序列化的消息。
     * @return 序列化后的字符串。
     * @usage auto line = Message::serialize(Msg);
     */
    [[nodiscard]] static std::string serialize(const Message& Rec)
    {
        return std::format(
            "{}|{}|{}|{}|{}|{}|{}|{}|{}",
            Rec.Id,
            static_cast<unsigned>(Rec.Level),
            duration_cast<nanoseconds>(Rec.Timestamp.time_since_epoch()).count(),
            Rec.Line,
            Rec.Column,
            escapeField(Rec.CategoryName),
            escapeField(Rec.FileName),
            escapeField(Rec.FunctionName),
            escapeField(Rec.MessageText)
        );
    }

    /**
     * @brief 从序列化文本反序列化消息。
     * @param Str 序列化字符串。
     * @return 反序列化后的 message。
     * @usage auto Msg = Message::deserialize(line);
     */
    [[nodiscard]] static Message deserialize(const std::string& Str)
    {
        const auto fields = splitEscapedFields(Str);
        if (fields.size() != 9)
        {
            throw std::invalid_argument("Message::deserialize invalid field count");
        }

        Message out;
        out.Id = std::stoull(fields[0]);
        out.Level = static_cast<ELogLevel>(std::stoul(fields[1]));
        const auto ns_since_epoch = static_cast<std::int64_t>(std::stoll(fields[2]));
        out.Timestamp = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(ns_since_epoch)));
        out.Line = static_cast<uint_least32_t>(std::stoul(fields[3]));
        out.Column = static_cast<uint_least32_t>(std::stoul(fields[4]));
        out.CategoryName = unescapeField(fields[5]);
        out.FileName = unescapeField(fields[6]);
        out.FunctionName = unescapeField(fields[7]);
        out.MessageText = unescapeField(fields[8]);
        return out;
    }

private:
    Message(uint64_t Id, ELogLevel InLevel, std::string_view Cat, const std::source_location& CalledLoc, std::string_view Msg)
        : Level(InLevel)
        , CategoryName(Cat)
        , Id(Id)
        , FileName(CalledLoc.file_name())
        , FunctionName(CalledLoc.function_name())
        , Line(CalledLoc.line())
        , Column(CalledLoc.column())
        , MessageText(Msg)
        , Timestamp(std::chrono::system_clock::now())
    {
    }

    static std::string escapeField(std::string_view InputChars)
    {
        std::string output;
        output.reserve(InputChars.size());
        for (const char ch : InputChars)
        {
            switch (ch)
            {
            case '\\':
                output += "\\\\";
                break;
            case '|':
                output += "\\|";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                output.push_back(ch);
                break;
            }
        }
        return output;
    }

    static std::string unescapeField(std::string_view InputChars)
    {
        std::string output;
        output.reserve(InputChars.size());

        bool escaped = false;
        for (const char ch : InputChars)
        {
            if (!escaped)
            {
                if (ch == '\\')
                {
                    escaped = true;
                }
                else
                {
                    output.push_back(ch);
                }
                continue;
            }

            switch (ch)
            {
            case 'n':
                output.push_back('\n');
                break;
            case 'r':
                output.push_back('\r');
                break;
            case 't':
                output.push_back('\t');
                break;
            default:
                output.push_back(ch);
                break;
            }
            escaped = false;
        }

        if (escaped)
        {
            output.push_back('\\');
        }

        return output;
    }

    static std::vector<std::string> splitEscapedFields(const std::string& Line)
    {
        std::vector<std::string> out;
        std::string current;
        current.reserve(Line.size());

        bool escaped = false;
        for (const char ch : Line)
        {
            if (!escaped)
            {
                if (ch == '\\')
                {
                    escaped = true;
                    current.push_back(ch);
                }
                else if (ch == '|')
                {
                    out.push_back(current);
                    current.clear();
                }
                else
                {
                    current.push_back(ch);
                }
                continue;
            }

            escaped = false;
            current.push_back(ch);
        }

        out.push_back(current);
        return out;
    }

private:
    ELogLevel Level{ELogLevel::Temp};
    std::string CategoryName;
    uint64_t Id{0};
    std::string FileName;
    std::string FunctionName;
    uint_least32_t Line{0};
    uint_least32_t Column{0};
    std::string MessageText;
    system_clock::time_point Timestamp{};
};

class Logger
{
    using sink_callback = std::function<void(const Message&)>;
    using sink_list = std::vector<sink_callback>;

public:
	/**
	 * @brief Logger 运行模式。
	 * @usage getRunMode() 返回当前模式，可用于引擎在启动阶段校验配置。
	 */
    enum class ERunMode : uint8_t
    {
        BackgroundThread,
        ManualFramePump,
    };

    static constexpr std::size_t RingBufferCapacity = 8192;
    static constexpr std::size_t DefaultFrameBudgetMessages = 1024;
    static constexpr std::size_t DefaultBackgroundBatchMessages = 256;

    static Logger& self()
    {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 记录一条日志（自动采集调用位置）
     * @param InLevel 日志级别
    * @param InCategory 日志分类
     * @param Fmt 格式化模板
     * @param InArgs 格式化参数
     * @return
     * @usage Logger::log(ELogLevel::Info, "hp={} mp={}", hp, mp);
     */
    template <typename CategoryType, typename... InArgTypes>
    static void log(
        ELogLevel InLevel,
        Category<CategoryType> InCategory,
        std::format_string<InArgTypes...> Fmt,
        InArgTypes&&... InArgs
    )
    {
        self().logImpl<CategoryType>(
            InLevel, 
            InCategory, 
            std::source_location::current(), 
            Fmt, 
            std::forward<InArgTypes>(InArgs)...
        );
    }

    /**
     * @brief 以"手动帧泵"模式启动 logger。
     * @usage Logger::self().startManually();
     */
    Logger& startManually()
    {
        stop();

        bool expected = false;
        if (!IsRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        {
            return *this;
        }

        RunMode.store(ERunMode::ManualFramePump, std::memory_order_release);
        openLogFile();
        return *this;
    }

    /**
     * @brief 以后台线程模式启动 logger。
     */
    Logger& start()
    {
        stop();

        bool expected = false;
        if (!IsRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        {
            return *this;
        }

        RunMode.store(ERunMode::BackgroundThread, std::memory_order_release);
        openLogFile();
        WorkerThread = std::jthread(
            [this](std::stop_token StopToken) {
                doWorkLoop(StopToken); 
            }
        );
        return *this;
    }

	/**
	 * @brief 引擎每帧调用：在预算内批量消费并落盘。
	 * @param MaxMessages 本帧最多处理消息数。
	 * @param time_budget 本帧允许消耗的时间预算；为空则仅按消息数限制。
	 * @return true 表示本帧消费到了至少一条消息。
	 * @usage Logger::self().pumpFrame(512, std::chrono::microseconds(300));
	 */
    bool pumpFrame(
        std::size_t MaxMessages = DefaultFrameBudgetMessages,
        std::optional<std::chrono::nanoseconds> TimeBudget = std::nullopt)
    {
        if (!IsRunning.load(std::memory_order_acquire))
        {
            return false;
        }

        const auto begin = std::chrono::steady_clock::now();
        std::size_t consumed = 0;

        while (consumed < MaxMessages)
        {
            if (!consumeOnce())
            {
                break;
            }
            ++consumed;

            if (TimeBudget.has_value() && (std::chrono::steady_clock::now() - begin) >= *TimeBudget)
            {
                break;
            }
        }

        flushFileBuffer();
        return consumed > 0;
    }

    /**
     * @brief 停止 Logger 并尽力冲刷剩余日志。
     * @return 当前 Logger 引用。
     * @usage Logger::self().stop();
     */
    Logger& stop()
    {
        const bool was_running = IsRunning.exchange(false, std::memory_order_acq_rel);
        if (!was_running)
        {
            return *this;
        }

        if (WorkerThread.joinable())
        {
            WorkerThread.request_stop();
            WorkerThread.join();
        }

        flushRemainingMessages();
        flushFileBuffer();

        if (LogFile.is_open())
        {
            LogFile.flush();
            LogFile.close();
        }

        return *this;
    }

    /**
     * @brief 注册自定义 sink 回调。
     * @param sink 接收 Message 的回调。
     * @usage Logger::self().registerSink([](const Message& m){ std::println("{}", m.text()); });
     */
    void registerSink(sink_callback Sink)
    {
        if (!Sink)
        {
            return;
        }

        std::lock_guard lock(SinkUpdateMutex);
        auto current = std::atomic_load_explicit(&SinksSnapshot, std::memory_order_acquire);
        sink_list next = current ? *current : sink_list{};
        next.push_back(std::move(Sink));
        std::atomic_store_explicit(&SinksSnapshot, std::make_shared<const sink_list>(std::move(next)), std::memory_order_release);
    }

    /**
     * @brief 清空所有自定义 sink。
     * @return
     * @usage Logger::self().clearSinks();
     */
    void clearSinks()
    {
        std::lock_guard lock(SinkUpdateMutex);
        std::atomic_store_explicit(&SinksSnapshot, std::make_shared<const sink_list>(), std::memory_order_release);
    }

    /**
     * @brief 获取因队列满被丢弃的日志计数。
     * @return 丢弃消息数量
     * @usage auto dropped = Logger::self().dropMessages();
     */
    [[nodiscard]] std::uint64_t dropMessages() const noexcept
    {
        return DroppedMessages.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取当前日志文件路径。
     * @return 日志文件路径。
     * @usage auto path = Logger::self().getLogFilePath();
     */
    [[nodiscard]] std::filesystem::path getLogFilePath() const
    {
        return LogFilePath;
    }

    /**
     * @brief 查询当前运行模式。
     * @return 当前 run_mode。
     * @usage if (Logger::self().getRunMode() == Logger::ERunMode::ManualFramePump) { ... }
     */
    [[nodiscard]] ERunMode getRunMode() const noexcept
    {
        return RunMode.load(std::memory_order_acquire);
    }

private:
    Logger() : MsgQueue(RingBufferCapacity)
    {
        std::atomic_store_explicit(&SinksSnapshot, std::make_shared<const sink_list>(), std::memory_order_release);
        SerializedBuffer.reserve(64 * 1024);
    }

    ~Logger()
    {
        stop();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    template <typename CategoryType, typename... InArgTypes>
    void logImpl(
        ELogLevel InLevel,
        const Category<CategoryType>& InCategory,
        const std::source_location& InCodeLocation, 
        std::format_string<InArgTypes...> Fmt, 
        InArgTypes&&... InArgs
    )
    {
        // 生产者热路径：只做格式化+入队，消费/IO 全部在消费侧完成。
        if (!IsRunning.load(std::memory_order_acquire))
        {
            return;
        }

        auto formatted = std::vformat(Fmt.get(), std::make_format_args(InArgs...));
        auto Msg = Message::makeMessage(
            MessageUId.fetch_add(1, std::memory_order_relaxed), 
            InLevel, 
            InCategory, 
            InCodeLocation, 
            formatted
        );
        if (!MsgQueue.tryPush(std::move(Msg)))
        {
            DroppedMessages.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void doWorkLoop(const std::stop_token& StopToken)
    {
        // 后台线程模式：以批处理驱动消费，空闲时短暂休眠降低 CPU 占用。
        while (!StopToken.stop_requested())
        {
            if (!pumpFrame(DefaultBackgroundBatchMessages))
            {
                // 如果没有消息可消费, 稍微休眠以避免忙等待
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (!IsRunning.load(std::memory_order_acquire) && MsgQueue.isEmpty())
            {
                break;
            }
        }
    }

    bool consumeOnce()
    {
        // 消费顺序：先写文件缓冲，再分发 sink，确保默认落盘路径与外部 sink 观察到一致顺序。
        auto popped = MsgQueue.tryPop();
        if (!popped.has_value())
        {
            return false;
        }

        writeFile(*popped);
        dispatchToSinks(*popped);
        return true;
    }

    void flushRemainingMessages()
    {
        // 仅在 stop 或收尾流程调用，尽力清空队列中残留消息。
        while (consumeOnce())
        {
        }
    }

    void dispatchToSinks(const Message& Msg)
    {
        // 无锁读快照：注册/清理 sink 时替换快照，消费线程只读取不可变列表。
        const auto sinks = std::atomic_load_explicit(&SinksSnapshot, std::memory_order_acquire);
        if (!sinks)
        {
            return;
        }

        for (const auto& sink : *sinks)
        {
            if (sink)
            {
                sink(Msg);
            }
        }
    }

    void openLogFile()
    {
        // 当前采用按启动时间生成新文件策略，避免多次启动覆盖历史日志。
        const auto now = std::chrono::system_clock::now();
        const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
                               .count();

        const auto log_dir = std::filesystem::current_path() / "logs";
        std::error_code ec;
        std::filesystem::create_directories(log_dir, ec);

        LogFilePath = log_dir / std::format("potato_{}.log", stamp);
        LogFile.open(LogFilePath, std::ios::binary | std::ios::out | std::ios::trunc);
    }

    void writeFile(const Message& Msg)
    {
        // 将序列化结果先写入内存缓冲，以降低每条日志触发一次系统写调用的开销。
        if (!LogFile.is_open())
        {
            return;
        }

        SerializedBuffer.append(Message::serialize(Msg));
        SerializedBuffer.push_back('\n');

        // 达到阈值时批量写，减少系统调用次数。
        if (SerializedBuffer.size() >= FlushThresholdBytes)
        {
            flushFileBuffer();
        }
    }

    void flushFileBuffer()
    {
        // 批量刷盘：由帧泵、后台循环和 stop 流程触发。
        if (!LogFile.is_open() || SerializedBuffer.empty())
        {
            return;
        }

        LogFile.write(SerializedBuffer.data(), static_cast<std::streamsize>(SerializedBuffer.size()));
        SerializedBuffer.clear();
    }

private:
    RingBuffer<Message, std::allocator<Message>, ERingBufferPolicy::MPSC> MsgQueue;
    std::shared_ptr<const sink_list> SinksSnapshot;

    std::jthread WorkerThread;

    mutable std::mutex SinkUpdateMutex;

    std::ofstream LogFile;
    std::filesystem::path LogFilePath;
    std::string SerializedBuffer;
    std::size_t FlushThresholdBytes{32 * 1024};

    std::atomic<ERunMode> RunMode{ERunMode::BackgroundThread};
    std::atomic<bool> IsRunning{false};
    std::atomic<std::uint64_t> DroppedMessages{0};
};

} // namespace core