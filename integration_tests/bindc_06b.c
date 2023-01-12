#include "bindc_06b.h"

int32_t compare_array_element(int32_t value1, double value2, int32_t code) {
    switch( code ) {
        case 0:
            return value1 <= value2;
        case 1:
            return value1 >= value2;
        default:
            return 0;
    }
}
