#pragma once
#include <type_traits>

namespace core::wrappers
{
template<typename EnumType>
concept enum_flag = std::is_enum_v<EnumType> && std::is_unsigned_v<std::underlying_type_t<EnumType>>;

template<enum_flag EnumType>
struct Flags {
    using underlying = std::underlying_type_t<EnumType>;
    underlying Value = 0;

    constexpr Flags() noexcept = default;
    constexpr Flags(EnumType E) noexcept : Value(static_cast<underlying>(E)) {}
    explicit constexpr Flags(underlying V) noexcept : Value(V) {}

    constexpr Flags operator|(Flags Other) const noexcept { return Flags(Value | Other.Value); }
    constexpr Flags operator&(Flags Other) const noexcept { return Flags(Value & Other.Value); }
    constexpr Flags operator~() const noexcept { return Flags(~Value); }

    constexpr bool operator==(Flags Other) const noexcept { return Value == Other.Value; }
    constexpr bool operator!=(Flags Other) const noexcept { return Value != Other.Value; }

    constexpr explicit operator bool() const noexcept { return Value != 0; }
    constexpr bool has(EnumType E) const noexcept { return (Value & static_cast<underlying>(E)) != 0; }
    constexpr void set(EnumType E) noexcept { Value |= static_cast<underlying>(E); }
    constexpr void clear(EnumType E) noexcept { Value &= ~static_cast<underlying>(E); }
};

// 枚举 | 枚举
template<enum_flag EnumType>
constexpr Flags<EnumType> operator|(EnumType Lhs, EnumType Rhs) noexcept {
    return Flags<EnumType>(Lhs) | Rhs;
}

// 枚举 | Flags
template<enum_flag EnumType>
constexpr Flags<EnumType> operator|(EnumType Lhs, Flags<EnumType> Rhs) noexcept {
    return Flags<EnumType>(Lhs) | Rhs;
}

// Flags | 枚举
template<enum_flag EnumType>
constexpr Flags<EnumType> operator|(Flags<EnumType> Lhs, EnumType Rhs) noexcept {
    return Lhs | Flags<EnumType>(Rhs);
}
}