#include "jltop.h"

static void init_modelinfo();
static void display_theader(int row);
static void draw_boxes(int row, int rowb);
static void display_command_bar();
static void init_colors();
static void init_view();
static void free_view();
static void view_controller();

static void init_stats();
static void update_stats();
static void print_stats();
static void free_stats();

static struct sys_stats stats;

static void draw_boxes(int row, int rowb) {
    move(row , 1);
    hline(ACS_HLINE, COLS - (WIDTH_TEMP + WIDTH_POWER + 2));
    move(row + 1, 0);
    vline(ACS_VLINE, rowb-row);
    move(row + 1, COLS - (WIDTH_TEMP + WIDTH_POWER + 1));
    vline(ACS_VLINE, rowb-row);
    move(row + 1, COLS - (WIDTH_POWER + 1));
    vline(ACS_VLINE, rowb-row);
    move(row + 1, COLS - 1);
    vline(ACS_VLINE, rowb-row);
    move(rowb+1, 1);
    hline(ACS_HLINE, COLS - 2);

    move(row, 0);
    addch(ACS_ULCORNER);
    move(row, COLS - (WIDTH_TEMP + WIDTH_POWER + 1));
    addch(ACS_TTEE);
    move(row, COLS - (WIDTH_POWER + 1));
    addch(ACS_TTEE);
    move(row, COLS - 1);
    addch(ACS_URCORNER);

    move(rowb+1, 0);
    addch(ACS_LLCORNER);
    move(rowb+1, COLS - (WIDTH_TEMP + WIDTH_POWER + 1));
    addch(ACS_BTEE);
    move(rowb+1, COLS - (WIDTH_POWER + 1));
    addch(ACS_BTEE);
    move(rowb+1, COLS - 1);
    addch(ACS_LRCORNER);
}

void linear_gauge(int y, int x, int size, struct gauge_values values) {
    int i, from, to;
    int boxwidth;
    float total_percent;

    total_percent =
        (values.percent1 + (values.percent2 >= 0.0 ? values.percent2 : 0.0) +
         (values.percent3 >= 0.0 ? values.percent3 : 0.0));
    boxwidth = size - 2 - strlen(values.name) - strlen(values.label);

    move(y, x);
    attrset(A_BOLD | COLOR_PAIR(4));
    addstr(values.name);
    attrset(A_BOLD);
    addch('[');
    attrset(A_NORMAL);
    if (values.status) {
        for (i = 0; i < boxwidth-strlen(values.value); i++)
            addch((float)i / (float)boxwidth < total_percent ? values.bar
                                                             : ' ');
        move(y, x + strlen(values.name) + 1 + boxwidth - strlen(values.value));
        addstr(values.value);
        for (i=0;i<boxwidth;i++) {
            move(y, x + strlen(values.name) + 1 + i);
            if (i <= (int)(values.percent1 * (float)boxwidth))
                chgat(1, values.attr1, values.color1, NULL);
            else if (i <= (int)((values.percent1+values.percent2) * (float)boxwidth))
                chgat(1, values.attr2, values.color2, NULL);
            else if (i <= (int)((values.percent1+values.percent2+values.percent3) * (float)boxwidth))
                chgat(1, values.attr3, values.color3, NULL);
        }
    } else {
        for (i = 0; i < boxwidth; i++)
            addch(' ');
        move(y, x + (boxwidth / 2) + 1);
        attrset(A_BOLD | COLOR_PAIR(1));
        printw("OFF");
        attrset(A_NORMAL);
    }
    move(y, x + strlen(values.name) + 1 + boxwidth);
    attrset(A_BOLD);
    addch(']');
    attrset(A_NORMAL);
    addstr(values.label);
}

static void display_theader(int row) {
    time_t clk;
    struct tm *cur_time;
    clk = time(NULL);
    cur_time = gmtime(&clk);
    move(row, 0);
    sprintf(viewbuf, "%s   %s %i cores",
            stats.model_name ? stats.model_name : "Unknown platform",
            stats.cstats->model_name ? stats.cstats->model_name : "Unknown processor",
            stats.cstats->numcpu
    );
    addstr(viewbuf);

    sprintf(viewbuf, "%4d/%02d/%02d %02d:%02d:%02d UTC",
            1900 + cur_time->tm_year, 1 + cur_time->tm_mon, cur_time->tm_mday,
            cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
    move(0, COLS - (strlen(viewbuf) + 1));
    addstr(viewbuf);
}

static void display_command_bar() {
    int i;
    move(LINES - 1, 0);
    attron(A_REVERSE);
    for (i = 0; i < COLS; i++)
        addch(' ');
    move(LINES - 1, 2);
    attroff(A_REVERSE);
    attron(A_BOLD);
    addch('Q');
    attroff(A_BOLD);
    attron(A_REVERSE);
    addstr("uit");
    attroff(A_REVERSE);
}

static void init_colors() {
    start_color();
    use_default_colors();
    init_pair(0, COLOR_BLACK, -1);
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_WHITE, -1);
}

static void init_view() {
    initscr();
    init_colors();
    curs_set(0);
    noecho();
    timeout(100);
}

static void free_view() {
    clear();
    refresh();
    endwin();
}

static void draw_view() {
    int i;
    int row, row_temp, row_power, rowb;
    int width = COLS - (WIDTH_TEMP + WIDTH_POWER + 3);
    if (!stats.is_updated)
        return;
    erase();
    display_theader(0);
    row = display_cpu_stats(1, stats.cstats);
    display_load_stats(row+1, width, stats.cstats);
    display_mem_stats(row+2, width, stats.mstats);
    display_gpu_stats(row+4, width, stats.gstats);
    display_fan_stats(row+5, width, stats.fstats);
    row_temp = display_temp_stats(row, COLS - WIDTH_TEMP - WIDTH_POWER - 1, WIDTH_TEMP,
                       stats.tstats);
    row_power = display_power_stats(row, COLS - WIDTH_POWER - 1, WIDTH_POWER,
                        stats.pstats);
    rowb = row+5 > row_temp ? row+5 : row_temp;
    rowb = rowb > row_power ? rowb : row_power;
    draw_boxes(row, rowb);
    display_command_bar();
    refresh();
}

static void view_controller() {
    int i;
    int row;
    int interval = 1000;
    char ch;
    while (true) {
        ch = wgetch(stdscr);
        if (ch == 'q')
            break;
        if (ch == KEY_RESIZE) {
            refresh();
            draw_view();
        }
        if (interval == 0) {
            update_stats();
            draw_view();
            interval = 1000;
        }
        interval -= 100;
    }
    return;
}

static void init_modelinfo() {
    FILE *f;
    stats.model_name = NULL;
    f = fopen(PATHMODELNAME, "r");
    if (f) {
        if (fgets(linebuf, 255, f)) {
            stats.model_name = malloc(strlen(linebuf) + 1 * sizeof(char));
            strcpy(stats.model_name, linebuf);
        }
        fclose(f);
    }
}

static void init_stats() {
    stats.is_updated = false;
    init_modelinfo();
    stats.cstats = malloc(sizeof(struct cpu_stats));
    stats.mstats = malloc(sizeof(struct mem_stats));
    stats.gstats = malloc(sizeof(struct gpu_stats));
    stats.fstats = malloc(sizeof(struct fan_stats));
    stats.pstats = malloc(sizeof(struct power_stats));
    stats.tstats = malloc(sizeof(struct temp_stats));

    init_cpu_stats(stats.cstats);
    init_gpu_stats(stats.gstats);
    init_fan_stats(stats.fstats);
    init_power_stats(stats.pstats);
    init_temp_stats(stats.tstats);
}

static void update_stats() {
    update_cpu_stats(stats.cstats);
    update_mem_stats(stats.mstats);
    update_gpu_stats(stats.gstats);
    update_fan_stats(stats.fstats);
    update_power_stats(stats.pstats);
    update_temp_stats(stats.tstats);
    stats.is_updated = true;
}

static void print_stats() {
    printf("%s\n\n", stats.model_name ? stats.model_name : "Unknown platform");
    print_cpu_stats(stats.cstats);
    print_gpu_stats(stats.gstats);
    print_fan_stats(stats.fstats);
    print_mem_stats(stats.mstats);
    print_power_stats(stats.pstats);
    print_temp_stats(stats.tstats);
}

static void free_stats() {
    free_cpu_stats(stats.cstats);
    free(stats.cstats);
    free(stats.mstats);
    free_gpu_stats(stats.gstats);
    free(stats.gstats);
    free_fan_stats(stats.fstats);
    free(stats.fstats);
    free_power_stats(stats.pstats);
    free(stats.pstats);
    free_temp_stats(stats.tstats);
    free(stats.tstats);
    if (stats.model_name)
        free(stats.model_name);
}

static void show_help() {
    printf("usage: jltop [-h] [-p]\n\n");
    printf("jltop is a system monitor for the Nvidia Jetson platform on the "
           "terminal\n\n");
    printf("options:\n");
    printf("  -h     show this help message and exit\n");
    printf("  -p     show system stats directly on terminal\n");
}

int main(int argc, char *argv[]) {
    int n, m, x, l, ch;
    bool show_curses = true;
    for (n = 1; n < argc; n++) {
        switch ((int)argv[n][0]) {
        case '-':
            x = 0;
            l = strlen(argv[n]);
            for (m = 1; m < l; ++m) {
                ch = (int)argv[n][m];
                switch (ch) {
                case 'p':
                    show_curses = false;
                    break;
                case 'h':
                    show_help();
                    exit(0);
                default:
                    printf("%s: invalid option -- '%c'\n", argv[0], ch);
                    printf("Try 'jltop -h' for more information.\n");
                    exit(1);
                }
            }
        }
    }
    init_stats();

    if (show_curses) {
        init_view();
        view_controller();
        free_view();
    } else {
        sleep(1);
        update_stats();
        print_stats();
    }
    free_stats();
    return 0;
}
