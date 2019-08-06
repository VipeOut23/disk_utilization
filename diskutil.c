#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#define HUMAN_READABLE 0x01
#define SHOW_FRACTION  0x02

#define IS_HUMAN_READABLE(f) (((f) & HUMAN_READABLE) == HUMAN_READABLE)
#define IS_SHOW_FRACTION(f)  (((f) & SHOW_FRACTION)  == SHOW_FRACTION)

/**
 * Returns total time spend in i/o (milliseconds)
 * @param stat opened stat file for block device
 */
uint64_t disk_time_io(FILE *stat)
{
        uint64_t dummy, io_ticks;

        /* io_ticks is the 10th stat entry */
        fscanf(stat, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
               &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
               &dummy, &dummy, &dummy, &io_ticks);

        return io_ticks;
}

/**
 * Output disk utilization
 * @param stat opened statfile for the block device
 * @param n times to show utilization (n<0 show infinite times)
 * @param flags output flags
  */
void output_loop(FILE *stat, int n, int flags)
{
        uint64_t io_then, io_now;
        float utilization;


        do {
                /* observer disk stats */
                if( fseek(stat, 0, SEEK_SET) == -1 ) {
                        perror("fseek");
                        return;
                }
                io_then = disk_time_io(stat);
                sleep(1);
                if( fseek(stat, 0, SEEK_SET) == -1 ) {
                        perror("fseek");
                        return;
                }
                io_now = disk_time_io(stat);
                utilization = (float)(io_now - io_then);


                /* output */
                if( IS_HUMAN_READABLE(flags) ) {
                        printf("%lu ", io_then);
                        printf("%lu ", io_now);
                        utilization /= 10.0;
                        if( IS_SHOW_FRACTION(flags) )
                                printf("%.2f%%\n", utilization);
                        else
                                printf("%.0f%%\n", utilization);
                }else {
                        utilization /= 1000.0;
                        printf("%f\n", utilization);
                }

                if(n>0) n--;
        }while(n);
}

int main()
{
        FILE *fp = fopen("/sys/block/sda/stat", "r");
        output_loop(fp, -1, HUMAN_READABLE);
        return 0;
}
