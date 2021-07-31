#pragma once

#include <type_traits>

enum class InputState : unsigned {
    None = 0,
    Fire1 = 1 << 0,
    Fire2 = 1 << 1,
    Fire3 = 1 << 2,
    Fire4 = 1 << 3,
    Start = 1 << 7,
};

constexpr InputState operator&(InputState x, InputState y)
{
    using UT = typename std::underlying_type_t<InputState>;
    return static_cast<InputState>(static_cast<UT>(x) & static_cast<UT>(y));
}

constexpr InputState operator|(InputState x, InputState y)
{
    using UT = typename std::underlying_type_t<InputState>;
    return static_cast<InputState>(static_cast<UT>(x) | static_cast<UT>(y));
}

constexpr InputState operator^(InputState x, InputState y)
{
    using UT = typename std::underlying_type_t<InputState>;
    return static_cast<InputState>(static_cast<UT>(x) ^ static_cast<UT>(y));
}

constexpr InputState operator~(InputState x)
{
    using UT = typename std::underlying_type_t<InputState>;
    return static_cast<InputState>(~static_cast<UT>(x));
}

inline InputState &operator&=(InputState &x, InputState y)
{
    return x = x & y;
}

inline InputState &operator|=(InputState &x, InputState y)
{
    return x = x | y;
}

inline InputState &operator^=(InputState &x, InputState y)
{
    return x = x ^ y;
}
