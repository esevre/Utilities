//
// Created by Erik Sevre on 11/4/2019.
//

#pragma once

#include <array>
#include <algorithm>


namespace ES::file {
    template <class Type>
    struct EndianChanger {
        union {
            Type data;
            std::array<char, sizeof(Type)> bytes;
        } value;

        constexpr
        Type swap() {
            std::reverse(value.bytes.begin(), value.bytes.end());
            return value.data;
        }
    };

    template <class Type>
    constexpr
    auto ChangeEndian(const Type &value) -> Type
    {
        EndianChanger<Type> ec {value};
        return ec.swap();
    }
}






