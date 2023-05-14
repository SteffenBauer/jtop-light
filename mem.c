#include "jltop.h"

#define PATH_MEMINFO "/proc/meminfo"
#define PATH_BUDDYINFO "/proc/buddyinfo"

void update_mem_stats(struct mem_stats *mstats) {
    FILE *f;
    mstats->mem_total = -1;
    mstats->mem_free = -1;
    mstats->mem_avail = -1;
    mstats->mem_buffers = -1;
    mstats->mem_cached = -1;
    mstats->mem_shared = -1;
    mstats->mem_reclaim = -1;
    mstats->swap_cached = -1;
    mstats->swap_total = -1;
    mstats->swap_free = -1;
    f = fopen(PATH_MEMINFO, "r");
    if (f) {
        while (fgets(linebuf, 255, f)) {
            if (sscanf(linebuf, "MemTotal: %d kB\n", &mstats->mem_total))
                continue;
            if (sscanf(linebuf, "MemFree: %d kB\n", &mstats->mem_free))
                continue;
            if (sscanf(linebuf, "MemAvailable: %d kB\n", &mstats->mem_avail))
                continue;
            if (sscanf(linebuf, "Buffers: %d kB\n", &mstats->mem_buffers))
                continue;
            if (sscanf(linebuf, "Cached: %d kB\n", &mstats->mem_cached))
                continue;
            if (sscanf(linebuf, "SwapCached: %d kB\n", &mstats->swap_cached))
                continue;
            if (sscanf(linebuf, "SwapTotal: %d kB\n", &mstats->swap_total))
                continue;
            if (sscanf(linebuf, "SwapFree: %d kB\n", &mstats->swap_free))
                continue;
            if (sscanf(linebuf, "Shmem: %d kB\n", &mstats->mem_shared))
                continue;
            if (sscanf(linebuf, "SReclaimable: %d kB\n", &mstats->mem_reclaim))
                continue;
        }
        fclose(f);
    }
}

static void scale_value(int *value, int *c) {
    for (*c = 0; *c < 4; (*c)++) {
        if (*value < 1024)
            break;
        *value /= 1024;
    }
}

void display_mem_stats(int row, int width, struct mem_stats *mstats) {
    char vbuf[32];
    float mem_total;
    float mem_used;
    float mem_perc_used;
    float mem_perc_buffers;
    float mem_perc_cache;
    float swap_total;
    float swap_used;
    float swap_perc;
    int it, iu;
    struct gauge_values memory_values = default_gauge;
    struct gauge_values swap_values = default_gauge;

    mem_total = (float)mstats->mem_total;
    mem_used =
        (float)(mstats->mem_total -
                (mstats->mem_free + mstats->mem_reclaim +
                mstats->mem_buffers + mstats->mem_cached));
    mem_perc_used = mem_used / mem_total;
    mem_perc_buffers = mstats->mem_buffers / mem_total;
    mem_perc_cache = mstats->mem_cached / mem_total;
    for (it = 0; it < 4; it++) {
        if (mem_total < 1024.0)
            break;
        mem_total /= 1024.0;
    }
    for (iu = 0; iu < 4; iu++) {
        if (mem_used < 1024.0)
            break;
        mem_used /= 1024.0;
    }
    sprintf(vbuf, "%1.1f%s/%1.1f%s", mem_used, MEMUNITS[iu], mem_total,
            MEMUNITS[it]);
    memory_values.name = "Mem  ";
    memory_values.value = vbuf;
    memory_values.label = "";
    memory_values.percent1 = mem_perc_used;
    memory_values.percent2 = mem_perc_buffers;
    memory_values.percent3 = mem_perc_cache;
    linear_gauge(row, 1, width, memory_values);

    swap_total = (float)mstats->swap_total;
    swap_used = (float)(mstats->swap_total - mstats->swap_free);
    swap_perc = swap_used / swap_total;
    for (it = 0; it < 4; it++) {
        if (swap_total < 1024.0)
            break;
        swap_total /= 1024.0;
    }
    for (iu = 0; iu < 4; iu++) {
        if (swap_used < 1024.0)
            break;
        swap_used /= 1024.0;
    }
    sprintf(vbuf, "%1.1f%s/%1.1f%s", swap_used, MEMUNITS[iu], swap_total,
            MEMUNITS[it]);
    swap_values.name = "Swap ";
    swap_values.value = vbuf;
    swap_values.label = "";
    swap_values.percent1 = swap_perc;
    swap_values.color1 = 1;
    linear_gauge(row + 1, 1, width, swap_values);
}

void print_mem_stats(struct mem_stats *mstats) {
    char vbuf[32];
    int value;
    int c = 0;

    printf(
        "            total        used        free      shared  buff/cache\n");
    printf("Mem: ");
    value = mstats->mem_total;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->mem_total - (mstats->mem_free + mstats->mem_reclaim + mstats->mem_buffers + mstats->mem_cached);
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->mem_free;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->mem_shared;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->mem_buffers + mstats->mem_cached;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    printf("\n");

    printf("Swap:");
    value = mstats->swap_total;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->swap_total - mstats->swap_free;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    value = mstats->swap_free;
    scale_value(&value, &c);
    printf(" %10i%s", value, MEMUNITS[c]);
    printf("\n\n");
}
