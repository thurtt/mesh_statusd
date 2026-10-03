#include "defs.h"
#include <stdbool.h>
#include <unistd.h>

#include <stdlib.h>
#include <math.h>


// global-ish vars
bool shutdown_complete = false;
bool daemon_mode = false;

void show_help()
{
    printf("\nmesh_statusd command line help\n");
    printf("----------------------------\n\n");
    printf("Usage:\n");
    printf("  mesh_statusd\n");
    printf("  mesh_statusd -d\n");
    printf("  mesh_statusd -s\n");
    printf("  mesh_statusd -h\n\n");
    printf("Options:\n");
    printf("-d     Start mesh_statusd as a daemon\n");
    printf("-s     Shut down the currently running daemon\n");
    printf("-h     Show this screen\n\n");
}

static void sig_handler(int signo)
{
    syslog(LOG_DEBUG, "Caught signal, exiting");
    if( daemon_mode == true )
    {
        remove(PID_FILE);
    }
    shutdown_display();
    exit(0);
}

// entrypoint
int main(int argc, char * argv[])
{
    int opt;
    int bytesread = 0;

    while ((opt = getopt(argc, argv, "dsh")) != -1) {
        switch (opt) {
        case 'd':
            daemon_mode = true;
            break;
        case 's':
            FILE * hFile = fopen(PID_FILE, "rt");
            if( hFile == NULL )
            {
                printf("Could not locate the pid at %s. You will need to kill the process manually", PID_FILE);
                exit(1);
            }
            char s_pid[10];
            memset(s_pid, 0, 10);
            bytesread = fread(s_pid, sizeof(char), 10, hFile);
            if( bytesread == 0 )
            {
                printf("Warning: 0 bytes read");
            }
            fclose(hFile);
            int pid = atoi(s_pid);
            printf("Shutting down pid: %d\n", pid);
            kill(pid, SIGTERM);
            exit(0);
        case 'h':
            show_help();
            exit(1);
        }
    }

    // register a handler for SIGINT and SIGTERM
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // run the daemon runner if in daemon mode
    if( daemon_mode == true )
    {
        printf("Starting mesh_statusd in daemon mode...\n");

        switch(fork())
        {
          case -1:
            return -1;
          case 0:
            int rc = daemon_runner();
            if( rc != 0 )
            {
                return rc;
            }
            break;
          default:
            exit(0);
        }
    }

    UBYTE *ImageBackground = init_display();
    if( ImageBackground == NULL )
    {
        return -1;
    }

    //int counter = 0;
    //char counter_str[255];

    struct DisplayData data;
    strncpy(data.svc_state, "Ok", sizeof(data.svc_state));
    data.cpu_load = 0;
    data.mem_usage = 0;
    data.gps_sync = true;
    data.nodes = 10;

    while(true)
    {
        cpu_mem_load(&data.cpu_load, &data.mem_usage);
        data.gps_sync = gps_sync();
        data.nodes = node_count();
        if ( mesh_state() )
        {
            strncpy(data.svc_state, "Ok", sizeof(data.svc_state));
        }
        else
        {
            strncpy(data.svc_state, "Down", sizeof(data.svc_state));
        }

        write_status_update(ImageBackground, data);
        sleep(1);
    }
}
