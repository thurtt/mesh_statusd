#include "defs.h"

void run_command(char *command, char * output, size_t output_size);

void cpu_mem_load(int * cpu_load, int * mem_usage)
{

    struct sysinfo si;
    if (sysinfo(&si) == -1) {
        perror("sysinfo");
        return;
    }

    // grab the one-minute load average across all CPUs
    double load1 = si.loads[0] / (double)(1 << SI_LOAD_SHIFT);
    int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count < 1)
    {
        cpu_count = 1;
    }

    double pct = load1 / cpu_count * 100.0;

    // clamp to 100%
    if (pct > 100.0)
    {
        pct = 100.0;
    }
    *cpu_load = round(pct);
    double used_ram = si.totalram - si.freeram;
    *mem_usage = round(used_ram / (double)si.totalram * 100.0);
}

bool gps_sync()
{
    char output[1024];
    run_command("python3 meshtastic_info.py --gps", output, sizeof(output));
    return strcmp(output, "ENABLED") == 0;
}

bool mesh_state()
{
    char output[1024];
    run_command("systemctl is-active meshtasticd", output, sizeof(output));
    if(strcmp(output, "active") == 0)
    {
        return true;
    }
    return false;
}

int node_count()
{
    char output[1024];
    run_command("python3 meshtastic_info.py --nodes", output, sizeof(output));
    return atoi(output);
}

void run_command(char *command, char * output, size_t output_size)
{
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    size_t bytesread = fread(output, 1, output_size, fp);

    if(output[bytesread - 1] == '\n')
    {
        output[bytesread - 1] = '\0';
    }
    pclose(fp);
}
