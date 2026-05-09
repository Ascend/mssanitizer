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


#ifndef SANITIZERS_NUMERIC_H
#define SANITIZERS_NUMERIC_H

#include <functional>
#include <type_traits>

namespace Sanitizer {

template<typename EnumT>
auto EnumToUnderlying(EnumT e) -> typename std::underlying_type<EnumT>::type
{
    using UnderlyingT = typename std::underlying_type<EnumT>::type;
    return static_cast<UnderlyingT>(e);
}

template<typename EnumT>
size_t HashEnum(EnumT e)
{
    // 获取枚举的底层类型（如 uint8_t）
    using hashUnderlyingT = typename std::underlying_type<EnumT>::type;
    // 将枚举值转换为底层类型后计算哈希
    return std::hash<hashUnderlyingT>{}(EnumToUnderlying(e));
}

template <typename T>
struct Hash {
    std::size_t operator()(T const &p) const
    {
        return std::hash<T>{}(p);
    }
};

template <typename T1, typename T2>
struct Hash<std::pair<T1, T2>> {
    std::size_t operator()(std::pair<T1, T2> const &p) const
    {
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};

} // namespace Sanitizer

#endif // SANITIZERS_NUMERIC_H

