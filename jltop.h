#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <glob.h>
#include <ncurses.h>

#define PATHMODELNAME "/proc/device-tree/model"
#define PATHCOMPATIBLE "/proc/device-tree/compatible"

#define WIDTH_TEMP (28)
#define WIDTH_POWER (38)

static struct cuda_table {
    char *gpu;
    char *cuda;
} CUDA_TABLE[] = {
    {"tegra23x", "8.7"}, // JETSON ORIN - tegra234
    {"tegra194", "7.2"}, // JETSON XAVIER
    {"tegra186", "6.2"}, // JETSON TX2
    {"tegra210", "5.3"}, // JETSON TX1
    {"tegra124", "3.2"}  // JETSON TK1
};

struct cputotal {
    int lasttotal, lastidle;
    float cur_load;
};

struct cpuinfo {
    bool active;
    int lasttotal, lastidle;
    float cur_load;
};

struct cpucluster {
    int num;
    int numcpus;
    int *cpus;
    char *current_governor;
    int cur_freq;
};

struct cpu_stats {
    char *model_name;
    int numcpu;
    int numclusters;
    struct cpucluster *cluster;

    struct cputotal cputotal;
    struct cpuinfo *cpu;
};

struct mem_stats {
    int mem_total;
    int mem_free;
    int mem_buffers;
    int mem_cached;
    int mem_avail;
    int mem_shared;
    int mem_reclaim;
    int swap_total;
    int swap_cached;
    int swap_free;
};

struct gpu_stats {
    char *gpu_dir;
    int load;
    int freq;
};

struct fan_stats {
    int pwm_hwmon;
    char *pwm_name;
    int pwm;

    int rpm_hwmon;
    int rpm;
};

struct temp_sensor {
    int temp;
    char *name;
};

struct temp_stats {
    int num;
    int nwidth;
    struct temp_sensor *sensor;
};

struct power_rail {
    int jp_version;
    int bus;
    char device[5];
    int channel;
    int rail;
    char *name;
    int current;
    int voltage;
    int power;
    int avg;
};

struct power_stats {
    int num;
    struct power_rail *power;
};

struct sys_stats {
    bool is_updated;
    char *model_name;
    struct cpu_stats *cstats;
    struct mem_stats *mstats;
    struct gpu_stats *gstats;
    struct fan_stats *fstats;
    struct power_stats *pstats;
    struct temp_stats *tstats;
};

struct gauge_values {
    char *name;
    char *value;
    char *label;
    float percent1;
    int attr1;
    int color1;
    float percent2;
    int attr2;
    int color2;
    float percent3;
    int attr3;
    int color3;
    bool status;
    char bar;
};

static struct gauge_values default_gauge = {
    .name = "",
    .value = "",
    .label = "",
    .percent1 = -1.0,
    .attr1 = A_BOLD,
    .color1 = 2,
    .percent2 = -1.0,
    .attr2 = A_BOLD,
    .color2 = 4,
    .percent3 = -1.0,
    .attr3 = A_NORMAL,
    .color3 = 3,
    .status = true,
    .bar = '|'
};

static char *FREQUNITS[] = {"kHz", "MHz", "GHz"};
static char *MEMUNITS[] = {"k", "M", "G", "T"};

static char viewbuf[256];
static char linebuf[256];
static char pathbuf[512];
static char namebuf[128];

void linear_gauge(int y, int x, int size, struct gauge_values values);

void init_cpu_stats(struct cpu_stats *cstats);
void update_cpu_stats(struct cpu_stats *cstats);
void free_cpu_stats(struct cpu_stats *cstats);
int display_cpu_stats(int row, struct cpu_stats *cstats);
int display_load_stats(int row, int width, struct cpu_stats *cstats);
void print_cpu_stats(struct cpu_stats *cstats);

void update_mem_stats(struct mem_stats *mstats);
void display_mem_stats(int row, int width, struct mem_stats *mstats);
void print_mem_stats(struct mem_stats *mstats);

void init_gpu_stats(struct gpu_stats *gstats);
void update_gpu_stats(struct gpu_stats *gstats);
void free_gpu_stats(struct gpu_stats *gstats);
void display_gpu_stats(int row, int width, struct gpu_stats *gstats);
void print_gpu_stats(struct gpu_stats *gstats);

void init_fan_stats(struct fan_stats *fstats);
void update_fan_stats(struct fan_stats *fstats);
void free_fan_stats(struct fan_stats *fstats);
void display_fan_stats(int row, int width, struct fan_stats *fstats);
void print_fan_stats(struct fan_stats *fstats);

void init_power_stats(struct power_stats *pstats);
void update_power_stats(struct power_stats *pstats);
void free_power_stats(struct power_stats *pstats);
int display_power_stats(int row, int col, int width,
                         struct power_stats *pstats);
void print_power_stats(struct power_stats *pstats);

void init_temp_stats(struct temp_stats *tstats);
void update_temp_stats(struct temp_stats *tstats);
void free_temp_stats(struct temp_stats *tstats);
int display_temp_stats(int row, int col, int width, struct temp_stats *tstats);
void print_temp_stats(struct temp_stats *tstats);