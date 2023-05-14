#include "jltop.h"

#define PATH_POWER_JP4 "/sys/bus/i2c/drivers/ina3221x"
#define PATH_POWER_JP5 "/sys/bus/i2c/drivers/ina3221"

static void add_power_rail(struct power_stats *pstats, int jp_version, int bus, char *i2cdev, int channel, int rail, char *label);
static void search_power_rails(struct power_stats *pstats);
static int cmp_rails (const void *a, const void *b);

static void add_power_rail(struct power_stats *pstats, int jp_version, int bus, char *i2cdev, int channel, int rail, char *label) {
    pstats->power = realloc(pstats->power, ++pstats->num * sizeof(struct power_rail));
    pstats->power[pstats->num - 1].name = malloc((1 + strlen(namebuf)) * sizeof(char));
    strcpy(pstats->power[pstats->num - 1].name, label);
    pstats->power[pstats->num - 1].jp_version = jp_version;
    pstats->power[pstats->num - 1].bus = bus;
    strcpy(pstats->power[pstats->num - 1].device, i2cdev);
    pstats->power[pstats->num - 1].channel = channel;
    pstats->power[pstats->num - 1].rail = rail;
    pstats->power[pstats->num - 1].current = 0;
    pstats->power[pstats->num - 1].voltage = 0;
    pstats->power[pstats->num - 1].power = -1;
    pstats->power[pstats->num - 1].avg = -1;
}

static void search_power_rails(struct power_stats *pstats) {
    FILE *f;
    int result;
    glob_t globbuf;
    int i;
    int bus;
    char i2cdev[5];
    int channel;
    int rail;

    if (!glob("/sys/bus/i2c/drivers/ina3221/*-*/hwmon/hwmon*/in*_enable", 0, NULL, &globbuf)) {
        for (i=0; i<globbuf.gl_pathc; i++) {
            if (sscanf(globbuf.gl_pathv[i], "/sys/bus/i2c/drivers/ina3221/%i-%4s/hwmon/hwmon%i/in%i_enable",
                   &bus, i2cdev, &channel, &rail) == 4) {
                sprintf(pathbuf, "/sys/bus/i2c/drivers/ina3221/%i-%4s/hwmon/hwmon%i/in%i_label", bus, i2cdev, channel, rail);
                f = fopen(pathbuf, "r");
                if (f) {
                    result = fscanf(f, "%s", namebuf);
                    fclose(f);
                    if (result == 1 && (strcmp(namebuf, "NC") != 0))
                        add_power_rail(pstats, 5, bus, i2cdev, channel, rail, namebuf);
                }
            }
        }
        globfree(&globbuf);
    }

    if (!glob("/sys/bus/i2c/drivers/ina3221x/*-*/iio:device*/rail_name_*", 0, NULL, &globbuf)) {
        for (i=0; i<globbuf.gl_pathc; i++) { 
            if (sscanf(globbuf.gl_pathv[i], "/sys/bus/i2c/drivers/ina3221x/%i-%4s/iio:device%i/rail_name_%i",
                   &bus, i2cdev, &channel, &rail) == 4) {
                f = fopen(globbuf.gl_pathv[i], "r");
                if (f) {
                    result = fscanf(f, "%s", namebuf);
                    fclose(f);
                    if (result == 1 && (strcmp(namebuf, "NC") != 0))
                        add_power_rail(pstats, 4, bus, i2cdev, channel, rail, namebuf);
                }
            }
        }
        globfree(&globbuf);
    }
}

static int cmp_rails (const void *a, const void *b) {
    struct power_rail *r1 = (struct power_rail*)a;
    struct power_rail *r2 = (struct power_rail*)b;
    if ((r1->rail == r2->rail) && (r1->channel == r2->channel)) return 0;
    if ((r1->rail > r2->rail) && (r1->channel > r2->channel)) return 1;
    return -1;
}

void init_power_stats(struct power_stats *pstats) {
    pstats->num = 0;
    pstats->power = NULL;
    search_power_rails(pstats);
    qsort(pstats->power, pstats->num, sizeof(struct power_rail), cmp_rails);
}

void update_power_stats(struct power_stats *pstats) {
    FILE *f;
    int i;
    for (i = 0; i < pstats->num; i++) {
        if (pstats->power[i].jp_version == 4)
            sprintf(pathbuf, "%s/%i-%4s/iio:device%i/in_current%i_input",
                PATH_POWER_JP4,
                pstats->power[i].bus,
                pstats->power[i].device,
                pstats->power[i].channel,
                pstats->power[i].rail
               );
        else
            sprintf(pathbuf, "%s/%i-%4s/hwmon/hwmon%i/curr%i_input",
                PATH_POWER_JP5,
                pstats->power[i].bus,
                pstats->power[i].device,
                pstats->power[i].channel,
                pstats->power[i].rail
               );
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &pstats->power[i].current) != 1)
                pstats->power[i].current = -1;
            fclose(f);
        }

        if (pstats->power[i].jp_version == 4)
            sprintf(pathbuf, "%s/%i-%4s/iio:device%i/in_voltage%i_input",
                PATH_POWER_JP4,
                pstats->power[i].bus,
                pstats->power[i].device,
                pstats->power[i].channel,
                pstats->power[i].rail
               );
        else
            sprintf(pathbuf, "%s/%i-%4s/hwmon/hwmon%i/in%i_input",
                PATH_POWER_JP5,
                pstats->power[i].bus,
                pstats->power[i].device,
                pstats->power[i].channel,
                pstats->power[i].rail
               );
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &pstats->power[i].voltage) != 1)
                pstats->power[i].voltage = -1;
            fclose(f);
        }

        if ((pstats->power[i].current != -1) &&
            (pstats->power[i].voltage != -1)) {
            pstats->power[i].power =
                (pstats->power[i].current * pstats->power[i].voltage) / 1000;
            if (pstats->power[i].avg < 0)
                pstats->power[i].avg = pstats->power[i].power;
            else
                pstats->power[i].avg =
                    (pstats->power[i].avg + pstats->power[i].power) / 2.0;
        } else {
            pstats->power[i].power = -1;
            pstats->power[i].avg = -1;
        }
    }
}

void free_power_stats(struct power_stats *pstats) {
    int i;
    for (i = 0; i < pstats->num; i++)
        free(pstats->power[i].name);
    free(pstats->power);
}

int display_power_stats(int row, int col, int width,
                         struct power_stats *pstats) {
    int i;
    int nstart, pstart, astart;
    char fbuf[32];
    move(row, col + 1);
    hline(ACS_HLINE, width - 1);
    nstart = 2;
    pstart = width - 18;
    astart = width - 9;
    move(row, col + nstart);
    addstr(" [Power/mW] ");
    move(row, col + pstart);
    addstr(" [Cur] ");
    move(row, col + astart);
    addstr(" [Avg] ");
    for (i = 0; i < pstats->num; i++) {
        move(row + i + 1, col + nstart + 1);
        printw("%s", pstats->power[i].name);
        move(row + i + 1, col + pstart + 1);
        if (pstats->power[i].power >= 0)
            printw("%5d", pstats->power[i].power);
        else
            printw(" N/A  ");
        move(row + i + 1, col + astart + 1);
        if (pstats->power[i].avg >= 0)
            printw("%5d", pstats->power[i].avg);
        else
            printw(" N/A  ");
    }
    return row+pstats->num;
}

void print_power_stats(struct power_stats *pstats) {
    int i;
    for (i = 0; i < pstats->num; i++) {
        printf("%16s: %5i mW %5i mW (avg)\n", pstats->power[i].name,
               pstats->power[i].power, pstats->power[i].avg);
    }
    printf("\n");
}
