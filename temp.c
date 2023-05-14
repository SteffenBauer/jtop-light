#include "jltop.h"

#define PATH_SENSOR_TYPE "/sys/devices/virtual/thermal/thermal_zone%i/type"
#define PATH_SENSOR_TEMP "/sys/devices/virtual/thermal/thermal_zone%i/temp"

#define TEMPERATURE_OFFLINE (-256000)

void init_temp_stats(struct temp_stats *tstats) {
    FILE *f;
    int i;
    char *sensor_name;
    tstats->num = 0;
    tstats->nwidth = 0;
    tstats->sensor = NULL;
    while (true) {
        sprintf(pathbuf, PATH_SENSOR_TYPE, tstats->num);
        f = fopen(pathbuf, "r");
        if (f) {
            tstats->sensor = realloc(
                tstats->sensor, ++tstats->num * sizeof(struct temp_sensor));
            tstats->sensor[tstats->num - 1].temp = TEMPERATURE_OFFLINE;
            if (fscanf(f, "%s", linebuf) == 1) {
                sensor_name = strtok(linebuf, "-_");
                tstats->sensor[tstats->num - 1].name =
                    malloc(sizeof(char) * (1 + strlen(sensor_name)));
                strcpy(tstats->sensor[tstats->num - 1].name, sensor_name);
            } else {
                tstats->sensor[tstats->num - 1].name = malloc(sizeof(char) * 4);
                strcpy(tstats->sensor[tstats->num - 1].name, "N/A");
            }
            fclose(f);
        } else
            break;
    }
    for (i = 0; i < tstats->num; i++)
        if (strlen(tstats->sensor[i].name) > tstats->nwidth)
            tstats->nwidth = strlen(tstats->sensor[i].name);
    return;
}

void update_temp_stats(struct temp_stats *tstats) {
    int i;
    FILE *f;
    for (i = 0; i < tstats->num; i++) {
        sprintf(pathbuf, PATH_SENSOR_TEMP, i);
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &tstats->sensor[i].temp) != 1)
                tstats->sensor[i].temp = TEMPERATURE_OFFLINE;
            fclose(f);
        }
    }
}

void free_temp_stats(struct temp_stats *tstats) {
    int i;
    for (i = 0; i < tstats->num; i++)
        free(tstats->sensor[i].name);
    free(tstats->sensor);
}

int display_temp_stats(int row, int col, int width,
                        struct temp_stats *tstats) {
    int i;
    int sstart, tstart;
    char fbuf[32];
    move(row, col + 1);
    hline(ACS_HLINE, width - 1);
    sstart = 4;
    tstart = width - 11;
    move(row, col + sstart);
    addstr(" [Sensor] ");
    move(row, col + tstart);
    addstr(" [Temp/C] ");
    sprintf(fbuf, "%%%ds", tstats->nwidth);
    for (i = 0; i < tstats->num; i++) {
        move(row + i + 1, col + sstart + 1);
        printw(fbuf, tstats->sensor[i].name);
        move(row + i + 1, col + tstart + 3);
        if (tstats->sensor[i].temp != TEMPERATURE_OFFLINE)
            printw("%4.1f", tstats->sensor[i].temp / 1000.0);
        else
            printw(" N/A ");
    }
    return row+tstats->num;
}

void print_temp_stats(struct temp_stats *tstats) {
    int i, temp;
    for (i = 0; i < tstats->num; i++) {
        temp = tstats->sensor[i].temp;
        if (temp == TEMPERATURE_OFFLINE)
            printf("%10s   N/A\n", tstats->sensor[i].name);
        else
            printf("%10s %5.1f C\n", tstats->sensor[i].name,
                   (float)temp / 1000.0);
    }
}