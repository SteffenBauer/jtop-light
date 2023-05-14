#include "jltop.h"

#define PATH_FANDIR "/sys/class/hwmon"
#define PATH_FNAME "/sys/class/hwmon/%s/name"
#define PATH_FANINFO "/sys/class/hwmon/hwmon%i/%s"

void init_fan_stats(struct fan_stats *fstats) {
    DIR *d;
    struct dirent *entry;
    FILE *f;
    int nhwmon;
    fstats->pwm_hwmon = -1;
    fstats->pwm_name = NULL;
    fstats->rpm_hwmon = -1;
    d = opendir(PATH_FANDIR);
    if (d) {
        while (entry = readdir(d)) {
            if (sscanf(entry->d_name, "hwmon%i", &nhwmon) == 1) {
                sprintf(pathbuf, PATH_FNAME, entry->d_name);
                f = fopen(pathbuf, "r");
                if (f) {
                    if (fscanf(f, "%s", linebuf)) {
                        /* Jetson Nano legacy fan */
                        if (strcmp(linebuf, "tegra_pwmfan") == 0) {
                            fstats->pwm_hwmon = nhwmon;
                            fstats->pwm_name = "cur_pwm";
                        }
                        /* Jetson Orin fan, PWM & RPM */
                        else if (strcmp(linebuf, "pwmfan") == 0) {
                            fstats->pwm_hwmon = nhwmon;
                            fstats->pwm_name = "pwm1";
                        } else if (strcmp(linebuf, "pwm_tach") == 0) {
                            fstats->rpm_hwmon = nhwmon;
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(d);
    }
}

void update_fan_stats(struct fan_stats *fstats) {
    FILE *f;
    fstats->pwm = -1;
    fstats->rpm = -1;
    if ((fstats->pwm_hwmon >= 0) && fstats->pwm_name) {
        sprintf(pathbuf, PATH_FANINFO, fstats->pwm_hwmon, fstats->pwm_name);
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &fstats->pwm) == 0)
                fstats->pwm = -1;
            fclose(f);
        }
    }
    if (fstats->rpm_hwmon >= 0) {
        sprintf(pathbuf, PATH_FANINFO, fstats->rpm_hwmon, "rpm");
        f = fopen(pathbuf, "r");
        if (f) {
            if (fscanf(f, "%d", &fstats->rpm) == 0)
                fstats->rpm = -1;
            fclose(f);
        }
    }
}

void free_fan_stats(struct fan_stats *fstats) {}

void display_fan_stats(int row, int width, struct fan_stats *fstats) {
    char vbuf[32];
    char lbuf[32];
    float fp;
    struct gauge_values fan_values = default_gauge;

    strcpy(vbuf, "");
    strcpy(lbuf, " N/A");
    fp = 0;
    if (fstats->pwm >= 0) {
        fp = (float)fstats->pwm / 255.0;
        sprintf(vbuf, "%4.0f%%", 100.0 * fp);
    }
    if (fstats->rpm >= 0) {
        sprintf(lbuf, " %4d RPM", fstats->rpm);
    } else if (fstats->pwm >= 0) {
        sprintf(lbuf, " %3d PWM", fstats->pwm);
    }
    fan_values.name = "FAN  ";
    fan_values.value = vbuf;
    fan_values.label = lbuf;
    fan_values.percent1 = fp;
    linear_gauge(row, 1, width, fan_values);
}

void print_fan_stats(struct fan_stats *fstats) {
    printf("Fan status: Current PWM %i (%1.0f%%), RPM %i\n", fstats->pwm,
           100.0 * ((float)fstats->pwm / 255.0), fstats->rpm);
}
