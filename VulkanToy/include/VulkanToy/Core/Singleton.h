//
// Created by ZHIKANG on 2023/3/25.
//

#pragma once

namespace VT
{
    template <class T>
    class Singleton
    {
    private:
        Singleton() = default;

    public:
        static T* Get()
        {
            // Thread safe after C++11
            static T singleton{};
            return &singleton;
        }
    };
}
