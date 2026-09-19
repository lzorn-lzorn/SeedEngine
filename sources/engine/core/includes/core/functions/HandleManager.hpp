#pragma once

#include <core/common/Global.hpp>
#include <string>
#include <functional>
#include <memory>
/**
 * HandleManager 是一个用于 Id 分配的全局管理器
 * 其可以给你的提供一个包装器 Handle, 使其具有一个全局唯一的标识并附带一种属性. 例如:
 *  - Name
 *  - Pointer Wrapper
 *  - System Tag
 */

namespace core
{

template <typename Tag>
struct TypedHandle
{
    /**
     * @note: Generation 的作用是: 在 Id 被复用时, 让旧句柄自动失效
     * @example:
     * ```cpp
        h1 = {Id=1, Gen=1};
        destroy(h1);
        h2 = {Id=1, Gen=2};
     * ```
     * 此时复用时 Generation++ , 旧的 h1 就会失效, 因为 h1.Gen != h2.Gen
     */
    HandleIdType         Id         = InvalidHandleId;
    HandleGenerationType Generation = InvalidHandleGeneration;

    constexpr TypedHandle() noexcept = default;

    constexpr TypedHandle(HandleIdType InId, HandleGenerationType InGen = 1) noexcept
        : Id(InId), Generation(InGen) {}

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        return Id != InvalidHandleId;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return isValid(); }

    constexpr bool operator==(const TypedHandle&) const noexcept = default;
    constexpr auto operator<=>(const TypedHandle&) const noexcept = default;

    constexpr void reset() noexcept
    {
        Id         = InvalidHandleId;
        Generation = InvalidHandleGeneration;
    }
};

struct IHandlePoolBase
{
    virtual ~IHandlePoolBase() = default;

    [[nodiscard]] virtual size_t size() const noexcept = 0;
    virtual void clear() noexcept = 0;
    [[nodiscard]] virtual bool isValidById(HandleIdType Id,
                                           HandleGenerationType Gen) const noexcept = 0;
};
template <typename Tag, typename T>
class HandlePool final : public IHandlePoolBase
{
public:
    using HandleType = TypedHandle<Tag>;

    HandlePool() = default;

    template <typename... Args>
    HandleType create(Args&&... InArgs)
    {
        HandleIdType Id;
        if (!FreeList.empty())
        {
            Id = FreeList.back();
            FreeList.pop_back();
            auto& Slot = Slots[Id - 1];
            Slot.Value = T(std::forward<Args>(InArgs)...);
            Slot.Generation++;
            Slot.IsAlive = true;
        }
        else
        {
            Id = static_cast<HandleIdType>(Slots.size() + 1);
            Slots.emplace_back();
            auto& Slot = Slots.back();
            Slot.Value = T(std::forward<Args>(InArgs)...);
            Slot.Generation = 1;
            Slot.IsAlive    = true;
        }
        return HandleType{ Id, Slots[Id - 1].Generation };
    }

    void destroy(HandleType H)
    {
        if (!isValid(H)) return;
        auto& Slot = Slots[H.Id - 1];
        Slot.IsAlive = false;
        Slot.Value   = T{};
        FreeList.push_back(H.Id);
    }

    [[nodiscard]] T* get(HandleType H) noexcept
    {
        if (!isValid(H)) return nullptr;
        return &Slots[H.Id - 1].Value;
    }
    [[nodiscard]] const T* get(HandleType H) const noexcept
    {
        if (!isValid(H)) return nullptr;
        return &Slots[H.Id - 1].Value;
    }

    [[nodiscard]] bool isValid(HandleType H) const noexcept
    {
        if (H.Id == InvalidHandleId || H.Id > Slots.size()) return false;
        const auto& Slot = Slots[H.Id - 1];
        return Slot.IsAlive && Slot.Generation == H.Generation;
    }

    // IHandlePoolBase
    [[nodiscard]] size_t size() const noexcept override
    {
        return Slots.size() - FreeList.size();
    }
    void clear() noexcept override
    {
        Slots.clear();
        FreeList.clear();
    }
    [[nodiscard]] bool isValidById(HandleIdType Id,
                                   HandleGenerationType Gen) const noexcept override
    {
        return isValid(HandleType{ Id, Gen });
    }

private:
    struct Slot
    {
        T                    Value{};
        HandleGenerationType Generation{ 0 };
        bool                 IsAlive{ false };
    };

    std::vector<Slot>         Slots;
    std::vector<HandleIdType> FreeList;
};




template <typename Ty, typename OwnerType>
struct NameHandle
{
    using owner_type = OwnerType;
    using holding_type = std::string;

    OwnerType* Owner = nullptr;
    HandleIdType Id = 0;
    std::string Name;
};

template <typename Ty, typename OwnerType>
struct SharedHandle
{
    using owner_type = OwnerType;
    using holding_type = Ty;

    OwnerType* Owner = nullptr;
    HandleIdType Id = 0;
    std::shared_ptr<Ty> Ptr = nullptr;

    constexpr SharedHandle() noexcept = default;

    SharedHandle(OwnerType* Owner, HandleIdType Id, std::shared_ptr<Ty> ptr) noexcept
        : Owner(Owner), Id(Id), Ptr(std::move(ptr)) {}

    template <typename DeleterType>
    SharedHandle(OwnerType* Owner, HandleIdType Id, Ty* Ptr, DeleterType Deleter = DeleterType()) noexcept
        : Owner(Owner), Id(Id), Ptr(Ptr, std::move(Deleter)) {}

    template <typename ...Arg>
    SharedHandle(OwnerType* Owner, HandleIdType Id, Arg... InParam) noexcept
        : Owner(Owner), Id(Id), Ptr(std::make_shared<Ty>(InParam...)) {}

    constexpr bool operator==(const SharedHandle& Rhs) const noexcept { return equalWith(Rhs); }
    constexpr std::strong_ordering operator<=>(const SharedHandle& Rhs) const noexcept { return Id <=> Rhs.Id; }

    constexpr bool equalWith(const SharedHandle& Rhs) const noexcept { return Id == Rhs.Id; }
    constexpr bool stronglyEqualWith(const SharedHandle& Rhs) const noexcept { return Id == Rhs.Id && Ptr == Rhs.Ptr; }
    bool isValid() const noexcept { return Id != InvalidHandleId && static_cast<bool>(Ptr);  }

    constexpr explicit operator bool() const noexcept { return isValid(); }
    constexpr bool empty() const noexcept { return !isValid(); }

    Ty* get() const noexcept { return Ptr.get(); }
    Ty& operator*() const { return *Ptr; }
    Ty* operator->() const noexcept { return Ptr.get(); }

    std::shared_ptr<Ty> const& shared() const noexcept { return Ptr; }
    std::shared_ptr<Ty> release() noexcept { return std::move(Ptr); }

    uint64_t getId() const noexcept { return Id; }
    uint64_t getUseCount() const noexcept { return Ptr.use_count(); }

    void swap(SharedHandle& Rhs) noexcept
    {
        using std::swap;
        swap(Id, Rhs.Id);
        swap(Ptr, Rhs.Ptr);
    }

    void reset() noexcept
    {
        Id = InvalidHandleId;
        Ptr.reset();
    }

    void reset(HandleIdType Id, std::shared_ptr<Ty> ptr) noexcept
    {
        this->Id = Id;
        Ptr = std::move(ptr);
    }
};

template <typename Ty>
struct UniqueHandle
{

    HandleIdType Id = 0;
    std::unique_ptr<Ty> Ptr;
};

template <typename OwnerType, typename HoldingType, typename DeleterType>
static SharedHandle<OwnerType, HoldingType> createSharedHandle(OwnerType* Owner, HoldingType* HoldingPointer, DeleterType Deleter = DeleterType()) noexcept
{
    return SharedHandle<OwnerType, HoldingType>(Owner, ++GNextHandleId, HoldingPointer, Deleter);
}
using GenericHandle = TypedHandle<Global_t>;
} // namespace core


namespace std
{
template<>
struct hash<::core::GenericHandle>
{
    ::std::size_t operator()(const ::core::GenericHandle& h) const noexcept
    {
        return ::std::hash<::core::HandleIdType>{}(h.Id);
    }
};
}

