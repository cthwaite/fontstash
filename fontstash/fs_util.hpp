#pragma once

namespace fontstash {
    static unsigned int hashint(unsigned int a)
    {
        a += ~(a<<15);
        a ^=  (a>>10);
        a +=  (a<<3);
        a ^=  (a>>6);
        a += ~(a<<11);
        a ^=  (a>>16);
        return a;
    }

    static int mini(int a, int b)
    {
        return a < b ? a : b;
    }

    static int maxi(int a, int b)
    {
        return a > b ? a : b;
    }
}