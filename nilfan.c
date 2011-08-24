#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define CONF "/etc/nilfan.conf"

#define MANUAL_SET "/sys/devices/platform/applesmc.768/fan1_manual"
#define FAN_OUTPUT "/sys/devices/platform/applesmc.768/fan1_output"

#define CPUINFO "/proc/cpuinfo"
#define TEMP_PATH_PRE "/sys/devices/platform/coretemp.0/temp"
#define TEMP_PATH_SUF "_input"

#define ERROR 1
#define SUCCESS 0

typedef struct
{
    unsigned short old_speed;
    unsigned short speed;
    unsigned short max_speed;
    unsigned short min_speed;
} Fan;

typedef struct 
{
    Fan fan;
    unsigned short old_temp;
    unsigned short temp;
    unsigned short low_temp;
    unsigned short high_temp;
    unsigned short cores;
    unsigned short polling_interval;
} Machine;

/* Prototypes */
void signal_handler(int);
void set_manual(int);
void log_fan_speed(unsigned short, unsigned short);
int get_core_temp(int);
unsigned short get_temp(unsigned short);
unsigned short calc_fan_speed(Machine, double A, double k);
void set_fan_speed(int);
void init(Machine*);
int main();

/* Handles any system signals */
void signal_handler(int signal)
{
    switch(signal)
    {
    case SIGHUP:
        break;
    case SIGTERM:
        set_manual(0);
        syslog(LOG_INFO, "Stop");
        closelog();
        exit(SUCCESS);
        break;
    }
}

/* Logs when the program changes fan speed */
void log_fan_speed(unsigned short speed, unsigned short temp)
{
    syslog(LOG_INFO, "Change: fan speed %d RPM temperature %d degrees celsius", speed, temp);
}

/* Query the number of cpu cores of the machine */
unsigned short query_cores()
{
    int cores;
    FILE *file;
    if((file = fopen(CPUINFO, "r")) != NULL)
    {
        char found = 0;
        size_t buf_size = 128;
        char *buf = (char*)malloc(sizeof(char)*buf_size+1);
        while(!found && getline(&buf, &buf_size, file) != -1)
        {
            if(strcmp(strtok(buf,"\t :\n"), "cpu") == 0 && 
                strcmp(strtok(NULL,"\t :\n"), "cores") == 0)
            {
                sscanf(strtok(NULL,"\t :\n"), "%d", &cores);
                found = 1;
            }
        }
        free(buf);
        fclose(file);
    }
    else 
    {
        syslog(LOG_ERR, "Error couldn't determine number of cpu cores");
        closelog();
        exit(ERROR);
    }
    return (unsigned short)cores;
}

/* Sets/clears manual fan control */
void set_manual(int set)
{
    FILE *file;
    if((file = fopen(MANUAL_SET, "w")) != NULL) 
    {
        fprintf(file, "%d", set);
        fclose(file);
    }
    else
    {
        syslog(LOG_ERR,"Error couldn't set manual fan control");
        closelog();
        exit(ERROR);
    }
}

/* Get the temperature of a cpu core */
int get_core_temp(int core)
{
    FILE *file;
    char *path = (char*)malloc(sizeof(char)*128);
    int temp;
    sprintf(path, "%s%d%s", TEMP_PATH_PRE, core, TEMP_PATH_SUF);
    if((file = fopen(path, "r")) != NULL)
    {
        fscanf(file, "%d", &temp);
        fclose(file);
    }
    else
    {
        syslog(LOG_ERR, "Error couldn't get temperature of core #%d", core);
        closelog();
        exit(ERROR);
    }
    free(path);
    return temp;
}

/* Get the average temperature of the cpu cores */
unsigned short get_temp(unsigned short cores)
{
    int sum = 0, index;
    for(index = 2; index < cores+2; index++)
        sum += get_core_temp(index);
    return (unsigned short)(sum / (cores * 1000));
}

/* Calculates the fan speed */
unsigned short calc_fan_speed(Machine mach, double A, double k)
{
    if(mach.temp < mach.low_temp)
    {
        return mach.fan.min_speed;
    }
    else if(mach.temp > mach.high_temp)
    {
        return mach.fan.max_speed;
    }
    else
    {
        // Exponential
        return (unsigned short)ceil(A * exp(k * mach.temp));
        // Linear
        //return (mach.fan.max_speed-mach.fan.min_speed)/(mach.high_temp-mach.low_temp)*(mach.temp-mach.low_temp)+mach.fan.min_speed;
    }
}

/* Sets the fan speed */
void set_fan_speed(int fan_speed)
{
    FILE *file;
    if((file = fopen(FAN_OUTPUT, "w")) != NULL) 
    {
        fprintf(file, "%d", fan_speed);
        fclose(file);
    }
    else
    {
        syslog(LOG_ERR, "Error couldn't set fan speed");
        closelog();
        exit(ERROR);
    }
}

/* Configure nilfan */
void configure(Machine *mach)
{
    FILE *file;
    if((file = fopen(CONF, "r")) != NULL)
    {
        int low, high, min, max, poll;
        fscanf(file, "%d %d %d %d %d",
                    &low, &high, &min, &max, &poll);
        mach->low_temp         = (unsigned short)low;
        mach->high_temp        = (unsigned short)high;
        mach->fan.min_speed    = (unsigned short)min;
        mach->fan.max_speed    = (unsigned short)max;
        mach->polling_interval = (unsigned short)poll;
        mach->fan.old_speed    = 0;
        mach->cores = query_cores();
        fclose(file);    
    }
    else
    {
        syslog(LOG_ERR, "Error couldn't set fan parameters");
        closelog();
        exit(ERROR);
    }
}

/* Initializes the fan control */
void init(Machine *mach)
{
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);

    openlog("nilfan", LOG_PID, LOG_DAEMON);

    set_manual(1);

    configure(mach);

    syslog(LOG_INFO, "Start");
}

/* Main function */
int main()
{
    Machine mach;
    double A, k;

    init(&mach);

    k = (log(mach.fan.max_speed) - log(mach.fan.min_speed)) / (mach.high_temp - mach.low_temp);
    A = mach.fan.max_speed * exp(-(mach.high_temp * k));

    while(1)
    {
        mach.temp = get_temp(mach.cores);
        mach.fan.speed = calc_fan_speed(mach, A, k);
        if(mach.fan.old_speed != mach.fan.speed)
        {
            set_fan_speed(mach.fan.speed);
            log_fan_speed(mach.fan.speed, mach.temp);
            mach.fan.old_speed = mach.fan.speed;
            mach.old_temp = mach.temp;
        }
        sleep(mach.polling_interval);
    }

    return SUCCESS;
}
