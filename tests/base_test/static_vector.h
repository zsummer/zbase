#pragma once
#include <vector>

template<class _Ty, size_t _Size>
class StaticVector : public std::vector<_Ty>
{
public:
    StaticVector()
    {
        std::vector<_Ty>::reserve(_Size);
    }
};