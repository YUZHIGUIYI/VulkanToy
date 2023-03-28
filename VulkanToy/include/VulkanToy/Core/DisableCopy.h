//
// Created by ZHIKANG on 2023/3/25.
//

#pragma once

namespace VT
{
    class DisableCopy
    {
    public:
        DisableCopy() = default;
        ~DisableCopy() = default;
        DisableCopy(DisableCopy const&) = delete;
        DisableCopy &operator=(DisableCopy const&) = delete;
        DisableCopy(DisableCopy &&) = delete;
        DisableCopy &operator=(DisableCopy &&) = delete;
    };
}
