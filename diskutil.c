#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

#define HUMAN_READABLE 0x01
#define SHOW_FRACTION  0x02

#define IS_HUMAN_READABLE(f) (((f) & HUMAN_READABLE) == HUMAN_READABLE)
#define IS_SHOW_FRACTION(f)  (((f) & SHOW_FRACTION)  == SHOW_FRACTION)

/**
 * Returns total time spend in i/o (milliseconds)
 * @param stat opened stat file for block device
 * @return is -1 on error, otherwise total time spend in i/o (milliseconds)
 */
uint64_t disk_time_io(const int fd)
{
        uint64_t io_ticks;
        char     buf[512];
        ssize_t  nread;
        int      ret;


        nread = read(fd, buf, sizeof(buf));
        if(nread < 0) {
                perror("read");
                return -1;
        }
        if(nread == sizeof(buf)) {
                fputs("Stat file too big\n", stderr);
                return -1;
        }
        buf[nread] = '\0';

        /* io_ticks is the 10th stat entry */
        ret = sscanf(buf, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                     &io_ticks, &io_ticks, &io_ticks, &io_ticks, &io_ticks,
                     &io_ticks, &io_ticks, &io_ticks, &io_ticks, &io_ticks);
        if(ret < 10) {
                fputs("Invalid data read from statfile\n", stderr);
                return -1;
        }

        return io_ticks;
}

/**
 * Output disk utilization
 * @param fd opened fd of statfile for the block device
 * @param n times to show utilization (n<0 show infinite times)
 * @param interval measure interval
 * @param prefix output prefix (NULL if none)
 * @param flags output flags
  */
int output_loop(const int fd, int n, const struct timespec *interval,
                const char *prefix, const int flags)
{
        uint64_t io_then, io_now;
        float utilization;

        do {
                /* observer disk stats */
                if( lseek(fd, 0, SEEK_SET) == -1 ) {
                        perror("lseek()");
                        return -1;
                }
                io_then = disk_time_io(fd);
                if( nanosleep(interval, NULL) == -1 ) {
                        if( errno == EINTR )
                                continue;
                        else {
                                perror("nanosleep()");
                                return -1;
                        }
                }
                if( lseek(fd, 0, SEEK_SET) == -1 ) {
                        perror("lseek");
                        return -1;
                }
                io_now = disk_time_io(fd);

                /* Calculate avg. utilization */
                utilization = (float)(io_now - io_then);
                utilization /= (interval->tv_nsec/1000000L
                                + interval->tv_sec*1000L);

                /* output */
                if(prefix)
                        fputs(prefix, stdout); // Avoid '\n' of puts
                if( IS_HUMAN_READABLE(flags) ) {
                        utilization *= 100.0;
                        if( IS_SHOW_FRACTION(flags) )
                                printf("%.2f%%\n", utilization);
                        else
                                printf("%.0f%%\n", utilization);
                }else {
                        printf("%f\n", utilization);
                }

                if(n>0) n--;
        }while(n);

        return 0;
}

/**
 * Check if a disk name is sane (alpha numeric)
 * @param name the name
 * @param len maximum length of name
 * @return true if sane, false otherwise
 */
bool sane_disk_name(char *name, size_t len)
{
        while(*name != '\0' && len) {
                if((*name >= 'a' &&
                    *name <= 'z')||
                   (*name >= 'A' &&
                    *name <= 'Z')||
                   (*name >= '0' &&
                    *name <= '9')) {
                        name++;
                        continue;
                }
                return false;
        }

        return true;
}

void print_help(char *self) {
        printf("%s [-h] [-f] [-i <float>] [-c <int>] [-p <string>] <disk>\n"
               "\t --human    | -h   : Output in percent\n"
               "\t --fraction | -f   : When human, also print a fraction\n"
               "\t --interval | -i   : Update interval in seconds (default 1.0)\n"
               "\t --count    | -c   : # of output lines (default -1 = inf)\n"
               "\t --prefix   | -p   : Prefix string to print before each line\n",
                self);
}

int main(int argc, char **argv)
{
        /* Defaults */
        char *prefix = NULL;
        int flags = 0;
        int count = -1;
        struct timespec interval = {1,0};
        char *disk = "sda";
        char statpath[PATH_MAX] = "/sys/block/";

        char c;
        float f_interval;
        int longind = 0;
        const char *optstring = "hfi:c:p:";
        const struct option longopts[] = {
                {"human", no_argument, NULL, 'h'},
                {"fraction", no_argument, NULL, 'f'},
                {"interval", required_argument, NULL, 'i'},
                {"count", required_argument, NULL, 'c'},
                {"prefix", required_argument, NULL, 'p'},
                {"help", no_argument, NULL, -127},
                {NULL, no_argument, NULL, 0}
        };

        while( (c = getopt_long(argc, argv, optstring, longopts, &longind)) != -1 ) {
                switch(c) {
                case 'h': /* Human */
                        flags |= HUMAN_READABLE;
                        break;
                case 'f': /* Fraction */
                        flags |= SHOW_FRACTION;
                        break;
                case 'i': /* Interval */
                        f_interval = strtof(optarg, NULL);
                        interval.tv_sec = f_interval;
                        interval.tv_nsec = 1000000000L *
                                (f_interval - (float)interval.tv_sec);
                        break;
                case 'c': /* Count */
                        count = strtol(optarg, NULL, 10);
                        break;
                case 'p': /* Prefix */
                        prefix = optarg;
                        break;
                case -127: /* Help */
                        print_help(argv[0]);
                        return 0;
                        break;
                }
        }

        if(optind < argc) {

                disk = argv[optind];
                if(!sane_disk_name(disk, NAME_MAX)){
                        fputs("Unsafe diskname, aborting\n", stderr);
                        return 1;
                }
        } else
                fprintf(stderr, "No disk specified, defaulting to \"%s\"\n", disk);

        strncat(statpath, disk, PATH_MAX-1);
        strncat(statpath, "/stat", PATH_MAX-1);



        int fd = open(statpath, O_RDONLY);
        if(fd == -1) {
                switch(errno) {
                case ENOENT:
                        fprintf(stderr, "Disk \"%s\" not found\n", disk);
                        break;
                case EACCES:
                case EPERM:
                        fprintf(stderr, "Insufficient perimission for \"%s\"", statpath);
                        break;
                default:
                        perror("open()");
                }
                return 1;

        }

        return output_loop(fd, count, &interval, prefix, flags);
}
