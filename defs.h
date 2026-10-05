#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <math.h>

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "Debug.h"
#include "OLED_1in5_rgb.h"
#include "oled/Config/DEV_Config.h"
#include "oled/GUI/GUI_Paint.h"

// file handling defines
#define BUFFER_SIZE 1024
#define PID_FILE "/run/mesh_statusd.pid"

// timer defines
#define TIMER_INTERVAL 30

// display defines
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128

// thresholds
extern int CPU_THRESHOLD[];
extern int MEM_THRESHOLD[];

// daemon prototypes
int daemon_runner();

// display data struct
struct DisplayData {
    char svc_state[60];
    int nodes;
    bool gps_sync;
    int cpu_load;
    int mem_usage;
};

UBYTE * init_display();
void write_status_update(UBYTE *ImageBackground, struct DisplayData data);
void write_line(UBYTE *ImageBackground, const char *key, const char *value, uint16_t val_color, int lineno);
void shutdown_display();
int text_width(const char *text, sFONT * Font);

void cpu_mem_load(int * cpu_load, int * mem_usage);
bool gps_sync();
bool mesh_state();
int node_count();
