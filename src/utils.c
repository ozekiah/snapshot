#include <time.h>
#include <stdio.h>

void timestamp(char *buffer)
{
        time_t timer;
        struct tm *tm_info;
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y%m%_d%H%M%S", tm_info);
}
