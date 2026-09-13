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

using HandleIdType = uint64_t;
inline constexpr HandleIdType InvalidHandleId = 0ull;

struct Handle
{
    HandleIdType Id = 0;

    constexpr bool isValid() const noexcept { return Id != InvalidHandleId; }
    constexpr explicit operator bool() const noexcept { return isValid(); }
    constexpr bool operator==(const Handle&) const = default;
    constexpr std::strong_ordering operator<=>(const Handle& Rhs) const = default;
};


struct NameHandle
{
    HandleIdType Id = 0;
    std::string Name;
};

template <typename Ty>
struct SharedHandle
{
    HandleIdType Id = 0;
    std::shared_ptr<Ty> Ptr;

    constexpr SharedHandle() noexcept = default;

    SharedHandle(HandleIdType Id, std::shared_ptr<Ty> ptr) noexcept
        : Id(Id), Ptr(std::move(ptr)) {}

    template <typename DeleterType>
    SharedHandle(HandleIdType Id, Ty* Ptr, DeleterType Deleter = DeleterType()) noexcept
        : Id(Id), Ptr(Ptr, std::move(Deleter)) {}


    explicit SharedHandle(std::shared_ptr<Ty> ptr) noexcept
        : Id(InvalidHandleId), Ptr(std::move(ptr)) {}

    constexpr SharedHandle(std::nullptr_t) noexcept {}

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

} // namespace core

namespace std
{
template<>
struct hash<::core::Handle>
{
    ::std::size_t operator()(const ::core::Handle& h) const noexcept
    {
        return ::std::hash<::core::HandleIdType>{}(h.Id);
    }
};
}

