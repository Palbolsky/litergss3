#ifndef CGSS_ZINDEX_H
#define CGSS_ZINDEX_H

#include <vector>

namespace cgss {
    struct ZIndex {
        long z = 0;
        long index = 0;

        inline int sign() const {
            if (z != 0) {
                return z > 0 ? 1 : -1;
            }
            return index == 0 ? 0 : (index > 0 ? 1 : -1);
        }

        inline ZIndex operator+(const ZIndex& alter) const {
            return {z + alter.z, index + alter.index};
        }

        inline ZIndex operator-(const ZIndex& alter) const {
            return {z - alter.z, index - alter.index};
        }

        inline bool operator==(const ZIndex& alter) const {
            return z == alter.z && index == alter.index;
        }
        
        inline bool operator!=(const ZIndex& alter) const {
            return !operator==(alter);
        }

        inline bool operator>(const ZIndex& alter) const {
            if (z > alter.z) {
                return true;
            }
            return z == alter.z && index > alter.index;
        }

        inline bool operator<=(const ZIndex& alter) const {
            return !operator>(alter);
        }
        
        inline bool operator<(const ZIndex& alter) const {
            if (z < alter.z) {
                return true;
            }
            return z == alter.z && index < alter.index;
        }

        inline bool operator>=(const ZIndex& alter) const {
            return !operator<(alter);
        }
    };
}
#endif