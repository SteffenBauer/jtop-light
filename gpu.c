#include "jltop.h"

#define PATH_GPUINFO "/sys/devices/gpu.0/%s"
#define PATH_GPUFREQ "/sys/devices/gpu.0/devfreq/%s/cur_freq"

void init_gpu_stats(struct gpu_stats *gstats) {
    DIR *d;
    struct dirent *entry;
    FILE *f;
    gstats->gpu_dir = NULL;
    sprintf(pathbuf, PATH_GPUINFO, "devfreq");
    d = opendir(pathbuf);
    if (d) {
        while ((entry = readdir(d)))
            if (entry->d_name[0] != '.') {
                gstats->gpu_dir =
                    malloc((1 + strlen(entry->d_name)) * sizeof(char));
                strcpy(gstats->gpu_dir, entry->d_name);
            }
        closedir(d);
    }
}

void update_gpu_stats(struct gpu_stats *gstats) {
    FILE *f;
    gstats->load = 0;
    gstats->freq = -1;
    if (!gstats->gpu_dir)
        return;

    sprintf(pathbuf, PATH_GPUINFO, "load");
    f = fopen(pathbuf, "r");
    if (f) {
        if (fscanf(f, "%d", &gstats->load) != 1)
            gstats->load = 0;
        fclose(f);
    }
    sprintf(pathbuf, PATH_GPUFREQ, gstats->gpu_dir);
    f = fopen(pathbuf, "r");
    if (f) {
        if (fscanf(f, "%d", &gstats->freq) != 1)
            gstats->freq = -1;
        fclose(f);
    }
}

void free_gpu_stats(struct gpu_stats *gstats) {
    if (gstats->gpu_dir)
        free(gstats->gpu_dir);
}

void display_gpu_stats(int row, int width, struct gpu_stats *gstats) {
    char vbuf[32];
    char lbuf[32];
    float load;
    float freq;
    int i;
    struct gauge_values gpu_values = default_gauge;

    load = (float)gstats->load / 1000.0;

    sprintf(vbuf, "%2.0f%%", 100.0 * load);
    if (gstats->freq < 0)
        sprintf(lbuf, "          ");
    else {
        freq = (float)gstats->freq / 1000.0;
        for (i = 0; i < 3; i++) {
            if (freq < 1000.0)
                break;
            freq /= 1000.0;
        }
        sprintf(lbuf, " %5.1f%s", freq, FREQUNITS[i]);
    }
    gpu_values.name = "GPU  ";
    gpu_values.value = vbuf;
    gpu_values.label = lbuf;
    gpu_values.percent1 = load;
    linear_gauge(row, 1, width, gpu_values);
}

void print_gpu_stats(struct gpu_stats *gstats) {
    if (gstats->gpu_dir)
        printf("GPU at %s Load %i Freq %i\n\n", gstats->gpu_dir, gstats->load,
               gstats->freq);
    else
        printf("GPU not available\n\n");
}
