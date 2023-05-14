#include "jltop.h"

#define PATHCPUINFO "/proc/cpuinfo"
#define PATHCPUSTAT "/proc/stat"
#define PATHCPUFREQ "/sys/devices/system/cpu/cpufreq"
#define PATHCPUPRESENT "/sys/devices/system/cpu/present"
#define PATHCPUPOLICY "/sys/devices/system/cpu/cpufreq/policy%i/%s"
#define PATHCPUONLINE "/sys/devices/system/cpu/cpu%i/online"

static void init_read_cpunum(struct cpu_stats *cstats);
static void sort_clusters(struct cpu_stats *cstats);
static void cluster_get_related_cpus(struct cpucluster *cluster);
static void init_cpus(struct cpu_stats *cstats);
static void init_clusters(struct cpu_stats *cstats);
static void update_cpus(struct cpu_stats *cstats);
static void update_clusters(struct cpu_stats *cstats);
static void update_load(struct cpu_stats *cstats);

static void init_read_cpunum(struct cpu_stats *cstats) {
    FILE *f;
    int mincpu, maxcpu;
    cstats->numcpu = 0;
    f = fopen(PATHCPUPRESENT, "r");
    if (f) {
        if (fscanf(f, "%i-%i", &mincpu, &maxcpu) == 2) {
            cstats->numcpu = maxcpu + 1;
        }
        fclose(f);
    }
}

static void sort_clusters(struct cpu_stats *cstats) {
    struct cpucluster temp;
    int i, j;
    for (i = 0; i < cstats->numclusters; i++)
        for (j = i + 1; j < cstats->numclusters; j++)
            if (cstats->cluster[i].num > cstats->cluster[j].num) {
                temp = cstats->cluster[i];
                cstats->cluster[i] = cstats->cluster[j];
                cstats->cluster[j] = temp;
            }
}

static void cluster_get_related_cpus(struct cpucluster *cluster) {
    FILE *f;
    int acpu;
    cluster->numcpus = 0;
    cluster->cpus = NULL;
    sprintf(pathbuf, PATHCPUPOLICY, cluster->num, "related_cpus");
    f = fopen(pathbuf, "r");
    if (f) {
        while (!feof(f)) {
            if (fscanf(f, "%i", &acpu) > 0)
                cluster->cpus =
                    realloc(cluster->cpus, (++cluster->numcpus) * sizeof(int));
            cluster->cpus[cluster->numcpus - 1] = acpu;
        }
        fclose(f);
    }
}

static void init_clusters(struct cpu_stats *cstats) {
    DIR *d;
    struct dirent *entry;
    FILE *f;
    int n;

    cstats->numclusters = 0;
    cstats->cluster = NULL;
    d = opendir(PATHCPUFREQ);
    if (d) {
        /* Read cpufreq policy directories */
        while ((entry = readdir(d)))
            if (strncmp("policy", entry->d_name, 6) == 0)
                if (sscanf(entry->d_name, "policy%i", &n)) {
                    cstats->cluster =
                        realloc(cstats->cluster, (++cstats->numclusters) *
                                                     sizeof(struct cpucluster));
                    cstats->cluster[cstats->numclusters - 1].num = n;
                    cstats->cluster[cstats->numclusters - 1].cpus = NULL;
                    cstats->cluster[cstats->numclusters - 1].current_governor =
                        malloc(32 * sizeof(char));
                    strcpy(cstats->cluster[cstats->numclusters - 1]
                               .current_governor,
                           "");
                    cluster_get_related_cpus(
                        &cstats->cluster[cstats->numclusters - 1]);
                }
        closedir(d);
    }
    sort_clusters(cstats);
}

static void init_cpus(struct cpu_stats *cstats) {
    int i;
    FILE *f;

    cstats->model_name = malloc(4 * sizeof(char));
    strcpy(cstats->model_name, "N/A");

    f = fopen(PATHCPUINFO, "r");
    if (f) {
        while (fgets(linebuf, 255, f)) {
            if (strncmp(linebuf, "model name\t: ", 13) == 0) {
                linebuf[strcspn(linebuf, "\n")] = 0;
                cstats->model_name =
                    realloc(cstats->model_name,
                            (1 + strlen(linebuf + 13)) * sizeof(char));
                strcpy(cstats->model_name, linebuf + 13);
                break;
            }
        }
        fclose(f);
    }

    init_read_cpunum(cstats);
    cstats->cputotal.lasttotal = 0;
    cstats->cputotal.lastidle = 0;

    cstats->cpu = malloc(cstats->numcpu * sizeof(struct cpuinfo));
    for (i = 0; i < cstats->numcpu; i++) {
        cstats->cpu[i].active = false;
        cstats->cpu[i].lasttotal = 0;
        cstats->cpu[i].lastidle = 0;
    }
}

static void update_load(struct cpu_stats *cstats) {
    FILE *f;
    f = fopen(PATHCPUSTAT, "r");
    if (f) {
        int jiffy = 0;
        int total = 0;
        int idle = 0;
        int uc = 0;
        int cpu = -1;
        int nextcpu = 0;
        while (true) {
            if (fscanf(f, "%s", linebuf) == 0)
                break;
            if (strcmp("intr", linebuf) == 0)
                break;
            if (strcmp("cpu", linebuf) == 0) {
                cpu = -1;
                total = 0;
                idle = 0;
                uc = 0;
            } else if (sscanf(linebuf, "cpu%i", &nextcpu) > 0) {
                if (cpu == -1) {
                    if (cstats->cputotal.lasttotal)
                        cstats->cputotal.cur_load =
                            (1.0 -
                             ((float)(idle - cstats->cputotal.lastidle) /
                              (float)(total - cstats->cputotal.lasttotal)));
                    cstats->cputotal.lasttotal = total;
                    cstats->cputotal.lastidle = idle;
                } else {
                    if (cstats->cpu[cpu].lasttotal)
                        cstats->cpu[cpu].cur_load =
                            (1.0 -
                             ((float)(idle - cstats->cpu[cpu].lastidle) /
                              (float)(total - cstats->cpu[cpu].lasttotal)));
                    cstats->cpu[cpu].lasttotal = total;
                    cstats->cpu[cpu].lastidle = idle;
                }
                cpu = nextcpu;
                total = 0;
                idle = 0;
                uc = 0;
            } else if (sscanf(linebuf, "%i", &jiffy) > 0) {
                total += jiffy;
                if (uc++ == 3)
                    idle = jiffy;
            }
        }
        fclose(f);
        if (cstats->cpu[cpu].lasttotal)
            cstats->cpu[cpu].cur_load =
                (1.0 - ((float)(idle - cstats->cpu[cpu].lastidle) /
                        (float)(total - cstats->cpu[cpu].lasttotal)));
        cstats->cpu[cpu].lasttotal = total;
        cstats->cpu[cpu].lastidle = idle;
    }
}

static void update_cpus(struct cpu_stats *cstats) {
    int i;
    FILE *f;
    int online;

    /* CPU Online */
    for (i = 0; i < cstats->numcpu; i++) {
        cstats->cpu[i].active = true;
        sprintf(pathbuf, PATHCPUONLINE, i);
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &online) == 1)
                cstats->cpu[i].active = (online == 1);
            fclose(f);
        }
    }
}

static void update_clusters(struct cpu_stats *cstats) {
    int i;
    FILE *f;
    for (i = 0; i < cstats->numclusters; i++) {
        sprintf(pathbuf, PATHCPUPOLICY, cstats->cluster[i].num,
                "scaling_governor");
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%s", cstats->cluster[i].current_governor) != 1)
                strcpy(cstats->cluster[i].current_governor, "N/A");
            fclose(f);
        }
        sprintf(pathbuf, PATHCPUPOLICY, cstats->cluster[i].num,
                "scaling_cur_freq");
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &cstats->cluster[i].cur_freq) != 1)
                cstats->cluster[i].cur_freq = 0;
            fclose(f);
        }
    }
}

int display_cpu_stats(int row, struct cpu_stats *cstats) {
    int i, j;
    int y, x;
    float freq;
    int drawrow;
    char nbuf[16];
    char vbuf[32];
    char lbuf[64];
    struct gauge_values cpu_values = default_gauge;

    drawrow = row;

    /* Processor cluster policy governor & frequency */
    for (i = 0; i < cstats->numclusters; i++) {
        if (cstats->cluster[i].cur_freq <= 0)
            sprintf(viewbuf, " Cluster %i(%i) - %s - N/A ", i + 1,
                    cstats->cluster[i].num,
                    cstats->cluster[i].current_governor);
        else {
            freq = (float)cstats->cluster[i].cur_freq;
            for (j = 0; j < 3; j++) {
                if (freq < 1000.0)
                    break;
                freq /= 1000.0;
            }
            sprintf(viewbuf, " Cluster %i(%i) - %s - %5.1f%s ", i + 1,
                    cstats->cluster[i].num, cstats->cluster[i].current_governor,
                    freq, FREQUNITS[j]);
        }
        move(drawrow, 0);
        addch(i == 0 ? ACS_ULCORNER : ACS_LTEE);
        attrset(A_BOLD);
        addstr(viewbuf);
        attrset(A_NORMAL);
        hline(ACS_HLINE, COLS - strlen(viewbuf) - 2);
        move(drawrow, COLS - 1);
        addch(i == 0 ? ACS_URCORNER : ACS_RTEE);

        drawrow++;
        for (j = 0; j < (cstats->cluster[i].numcpus + 1) / 2; j++) {
            move(drawrow + j, 0);
            addch(ACS_VLINE);
            move(drawrow + j, COLS - 1);
            addch(ACS_VLINE);
        }
        cpu_values.label = "";
        for (j = 0; j < cstats->cluster[i].numcpus; j++) {
            int ncpu = cstats->cluster[i].cpus[j];
            sprintf(nbuf, "CPU%2d", ncpu);
            sprintf(vbuf, "%2.0f%%", 100.0 * cstats->cpu[ncpu].cur_load);
            cpu_values.name = nbuf;
            cpu_values.value = vbuf;
            cpu_values.percent1 = cstats->cpu[ncpu].cur_load;
            cpu_values.status = cstats->cpu[ncpu].active;
            y = drawrow + j % ((cstats->cluster[i].numcpus + 1) / 2);
            x = 1 + j / ((cstats->cluster[i].numcpus + 1) / 2) *
                        ((COLS + 1) / 2 - 1);
            linear_gauge(y, x, (COLS + 1) / 2 - 2, cpu_values);
        }
        drawrow += (cstats->cluster[i].numcpus + 1) / 2;
    }
    move(drawrow, 0);
    addch(ACS_LLCORNER);
    hline(ACS_HLINE, COLS - 2);
    move(drawrow, COLS - 1);
    addch(ACS_LRCORNER);
    return drawrow + 1;
}

int display_load_stats(int row, int width, struct cpu_stats *cstats) {
    int i, j;
    int y, x;
    float freq;
    char vbuf[32];
    struct gauge_values cpu_values = default_gauge;

    /* Total load gauge & processor model name */
    sprintf(vbuf, "%2.0f%%", 100.0 * cstats->cputotal.cur_load);
    cpu_values.name = "Load ";
    cpu_values.value = vbuf;
    cpu_values.label = "";
    cpu_values.status = true;
    cpu_values.percent1 = cstats->cputotal.cur_load;
    linear_gauge(row, 1, width, cpu_values);
}

void print_cpu_stats(struct cpu_stats *cstats) {
    int i, j;
    int ccpu;
    printf("CPU Model: %s\n", cstats->model_name);
    printf("Number of CPUS: %i\n", cstats->numcpu);
    printf("Total load: %6.2f%%\n\n", 100.0 * cstats->cputotal.cur_load);
    printf("Number of CPU Clusters: %i\n", cstats->numclusters);
    for (i = 0; i < cstats->numclusters; i++) {
        printf("Cluster %i(%i) Policy %s Related CPUs %i Freq %i\n", i,
               cstats->cluster[i].num, cstats->cluster[i].current_governor,
               cstats->cluster[i].numcpus, cstats->cluster[i].cur_freq);
        for (j = 0; j < cstats->cluster[i].numcpus; j++) {
            ccpu = cstats->cluster[i].cpus[j];
            if (cstats->cpu[ccpu].active)
                printf("%i: Load %6.2f%%  ", ccpu,
                       100.0 * cstats->cpu[ccpu].cur_load);
            else
                printf("%i: offline  ", ccpu);
        }
        printf("\n");
    }
    printf("\n");
}

void init_cpu_stats(struct cpu_stats *cstats) {
    init_cpus(cstats);
    init_clusters(cstats);
    update_cpus(cstats);
    update_load(cstats);
    update_clusters(cstats);
}

void update_cpu_stats(struct cpu_stats *cstats) {
    update_cpus(cstats);
    update_load(cstats);
    update_clusters(cstats);
}

void free_cpu_stats(struct cpu_stats *cstats) {
    int i;
    for (i = 0; i < cstats->numclusters; i++) {
        free(cstats->cluster[i].current_governor);
        free(cstats->cluster[i].cpus);
    }
    free(cstats->cluster);
    free(cstats->cpu);
    free(cstats->model_name);
}
