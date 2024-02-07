#include <linux/types.h>

struct dmp_stat {
    ulong    r_count;
    ulong    w_count;
    ulong r_avg_size;
    ulong w_avg_size;
    ulong t_avg_size;
};