/*
 *  Copyright (C) 2015 David Bigagli
 *
 * * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */

/* openlava job starter example for the integration with docker.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static char buf[BUFSIZ];

int
main(int argc, char **argv)
{
    char *cmd;
    char *container;
    char *jobId;
    FILE *fp;
    FILE *pf;
    time_t t;
    struct tm *tm;
    int cc;

    fp = fopen("starter.log", "a+");
    setbuf(fp, NULL);

    t = time(NULL);
    tm = localtime(&t);

    fprintf(fp, "%d:%d:%d:%d:%d starting\n",
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    /* This is the user command we are going to
     * execute and which is going to run inside
     * docker container
     */
    cmd = argv[1];

    /* This is the image docker will use to create
     * a container in which to run the job
     */
    container = getenv("LSB_DOCKER_CONTAINER");

    /* The jobId is used as the name of the container
     * so job control action can use jobId and use
     * docker command to operate on the container.
     */
    jobId = getenv("LSB_JOBID");

    /* This is the complete command line which is
     * to be run.
     */
    sprintf(buf, "docker run --name=%s %s %s", jobId, container, cmd);

    fprintf(fp, "%d:%d:%d:%d:%d starter runs: %s\n",
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
            tm->tm_min, tm->tm_sec, buf);

    /* open a pipe and run the command
     */
    pf = popen(buf, "r");

    /* We don't expect any output from docker
     * so we can just pause in wait4() till
     * the job is done. You can just wait in
     * fgets() if you like.
     */
    cc = pclose(pf);

    fprintf(fp, "%d:%d:%d:%d:%d run done with status %d\n",
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
            tm->tm_min, tm->tm_sec, cc);
    fclose(fp);

    return 0;
}
