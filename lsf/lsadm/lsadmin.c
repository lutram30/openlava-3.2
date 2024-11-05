/*
 * Copyright (C) 2015 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pwd.h>
#include "lsadmin.h"

#define NL_SETN 25

static int doLsCmd(int argc, char **argv);
static void limDebugReq(struct debugReq *pdebug, int num, char **host);
static void resDebugReq(struct debugReq *pdebug, int num, char **host);
static int lsadminDebug (int argc, char **argv, int opCode);
static int checkPerm(void);
static int exitDebug;
extern int startup( int argc, char **argv, int opCode);
extern int linux_optind;
extern int linux_opterr;

static char *cmdList[] =
{
    "reconfig",
    "ckconfig",
    "limrestart",
    "limstartup",
    "limshutdown",
    "limlock",
    "limunlock",
    "resrestart",
    "resstartup",
    "resshutdown",
    "reslogon",
    "reslogoff",
    "limdebug",
    "limtime",
    "resdebug",
    "restime",
    "help",
    "?",
    "quit",
    NULL
};

static char *cmdSyntax[] =
{
    "[-v] [-f]",
    "[-v]",
    "[-v] [-f] [ host_name ... | all ]",
    "[-f] [ host_name ... | all ]",
    "[-f] [ host_name ... | all ]",
    "[-l time_seconds] ",
    "",
    "[-f] [ host_name ... | all ]",
    "[-f] [ host_name ... | all ]",
    "[-f] [ host_name ... | all ]",
    "[ host_name ... | all ] [-c cpu_time]",
    "[ host_name ... | all ]",
    "[-c class_name] [-l debug_level] [-f logfile_name] [-o] [host_name ...]",
    "[-l timing_level] [-f logfile_name] [-o] [host_name ...]",
    "[-c class_name] [-l debug_level] [-f logfile_name] [-o] [host_name ...]",
    "[-l timing_level] [-f logfile_name] [-o] [host_name ...]",
    "[ command ...]",
    "[ command ...]",
    "",
    NULL
};

static char *cmdInfo[] = {
    "Reconfigure LSF",
    "Check configuration files",
    "Restart LIM (Load Information Manager)",
    "Startup LIM (Load Information Manager)",
    "Shut down LIM (Load Information Manager)",
    "Lock LIM on local host",
    "Unlock LIM on local host",
    "Restart RES (Remote Execution Server)",
    "Startup RES (Remote Execution Server)",
    "Shut down RES (Remote Execution Server)",
    "Log tasks executed by RES",
    "RES log off",
    "Debug LIM",
    "Debug LIM timing message",
    "Debug RES",
    "Debug RES timing message",
    "Get help on commands",
    "Get help on commands",
    "Quit",
    NULL
};

static int opCodeList[] =
{
    LIM_CMD_REBOOT,
    -1,
    LIM_CMD_REBOOT,
    -1,
    LIM_CMD_SHUTDOWN,
    0,
    -1,
    RES_CMD_REBOOT,
    -1,
    RES_CMD_SHUTDOWN,
    RES_CMD_LOGON,
    RES_CMD_LOGOFF,
    LIM_DEBUG,
    LIM_TIMING,
    RES_DEBUG,
    RES_TIMING,
    -1,
    -1,
    -1
};

int
main(int argc, char **argv)
{
    int cc,  myIndex;
    char *prompt = "lsadmin>";
    char line[MAXLINELEN];

    if (ls_initdebug(argv[0]) < 0) {
	ls_perror("ls_initdebug");
	exit(-1);
    }

    while ((cc = getopt(argc, argv, "Vh")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                exit(0);
            case 'h':
            default:
                cmdsUsage("lsadmin", cmdList, cmdInfo);
        }
    }
    if (argc > optind) {
        if ((myIndex=adminCmdIndex(argv[optind], cmdList)) == -1) {
            fprintf(stderr, "Invalid command <%s> \n", argv[optind]);
            cmdsUsage("lsadmin", cmdList, cmdInfo);
	}
	optind++;
	exit (doLsCmd (argc, argv));
    }

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
	if (fgets(line, MAXLINELEN, stdin) == NULL) {
	    printf("\n");
            exit(-1);
        }
        parseAndDo(line , doLsCmd);

    }
}


static int
doLsCmd (int argc, char *argv[])
{
    int cmdRet = 0, cc, myIndex;

    if ((myIndex = adminCmdIndex(argv[optind-1], cmdList)) == -1) {
        fprintf(stderr, "Invalid command <%s>. Try help\n", argv[optind - 1]);
            return -1;
    }
    switch (myIndex) {
        case LSADM_CKCONFIG :
            if ((argc == optind + 1) && strcmp(argv[optind], "-v") == 0)
                cc =  checkConf(1, 1);
            else if (argc == optind) {
                cc = checkConf(0, 1);
                if (cc == -1 || cc == -2)
                    if (getConfirm("Do you want to see detailed messages? [y/n] "));
                cc =  checkConf(1, 1);
            } else
                cmdRet = -2;
            if (cmdRet == 0 && cc != 0)
                cmdRet = -1;
            break;
        case LSADM_RECONFIG :
        case LSADM_LIMREBOOT :
        case LSADM_LIMSHUTDOWN :
            cmdRet = limCtrl(argc, argv, opCodeList[myIndex]);
            break;
        case LSADM_LIMSTARTUP :
            cmdRet = startup(argc, argv, myIndex);
            break;
        case LSADM_LIMLOCK :
            cmdRet = limLock(argc, argv);
            break;
        case LSADM_LIMUNLOCK :
            cmdRet = limUnlock(argc, argv);
            break;
        case LSADM_RESREBOOT :
        case LSADM_RESSHUTDOWN :
        case LSADM_RESLOGON :
        case LSADM_RESLOGOFF :
            cmdRet = resCtrl(argc, argv, opCodeList[myIndex]);
            break;
        case LSADM_LIMDEBUG:
        case LSADM_LIMTIME:
        case LSADM_RESDEBUG:
        case LSADM_RESTIME:
            cmdRet = lsadminDebug(argc, argv, opCodeList[myIndex]);
            break;
        case LSADM_RESSTARTUP :
            cmdRet = startup(argc, argv, myIndex);
            break;
        case LSADM_HELP :
        case LSADM_QES :
            cmdHelp(argc, argv, cmdList, cmdInfo, cmdSyntax);
            break;
        case LSADM_QUIT :
            exit(0);
        default :
            fprintf(stderr, "adminCmdIndex: Error");
            exit(-1);
    }
    if (cmdRet == -2)
	oneCmdUsage(myIndex, cmdList, cmdSyntax);

    fflush(stderr);
    return (cmdRet);

}

static int
lsadminDebug(int argc, char **argv, int opCode)
{
    struct hostInfo *hostInfo;
    char  opt[10];
    char *hosts[20];
    char *localHost;
    char *word;
    int fileOption = FALSE;
    int i,j, num, c;
    int   numHosts = 0;
    struct debugReq  debug;



    exitDebug = 0;
    debug.opCode = opCode ;
    debug.logClass = 0;
    debug.level = 0;
    debug.hostName = NULL;
    debug.options = 0;

    if (opCode == LIM_DEBUG || opCode == RES_DEBUG)
        strcpy(opt, "oc:l:f:");
    else if (opCode == LIM_TIMING || opCode == RES_TIMING)
        strcpy (opt, "ol:f:");
    else
        return -2;
    linux_optind = 1;
    linux_opterr = 1;
    if (strstr(argv[0],"lsadmin")) {
	linux_optind++;
    }

    while ((c = getopt(argc, argv, opt )) != EOF) {
        switch (c) {
            case 'c':
                while (optarg != NULL && (word = getNextWord_(&optarg))) {
                    if (strcmp(word, "LC_SCHED") == 0)
                        debug.logClass |= LC_SCHED;
                    if (strcmp(word, "LC_EXEC") == 0)
                        debug.logClass |= LC_EXEC;
                    if (strcmp(word, "LC_TRACE") == 0)
                        debug.logClass |= LC_TRACE;
                    if (strcmp(word, "LC_COMM") == 0)
                        debug.logClass |= LC_COMM;
                    if (strcmp(word, "LC_XDR") == 0)
                        debug.logClass |= LC_XDR;
                    if (strcmp(word, "LC_FAIRSHARE") == 0)
                        debug.logClass |= LC_FAIR;
                    if (strcmp(word, "LC_FILE") == 0)
                        debug.logClass |= LC_FILE;
                    if (strcmp(word, "LC_AUTH") == 0)
                        debug.logClass |= LC_AUTH;
                    if (strcmp(word, "LC_HANG") == 0)
                        debug.logClass |= LC_HANG;
                    if (strcmp(word, "LC_SIGNAL") == 0)
                        debug.logClass |= LC_SIGNAL;
                    if (strcmp(word, "LC_PIM") == 0)
                        debug.logClass |= LC_PIM;
                }
                if (debug.logClass == 0) {
                    fprintf(stderr, "Command denied. Invalid class name\n");
                    return -1;
                }
                break;
            case 'l':
                for (i=0;i<strlen(optarg);i++) {
                    if (!isdigit(optarg[i])) {
                        fprintf(stderr, "\
Command denied. Invalid level value\n");
                        return -1;
                    }
                }
                debug.level = atoi(optarg);
                if (opCode == LIM_DEBUG || opCode == RES_DEBUG) {
                    if (debug.level < 0 || debug.level > 3) {
                        fprintf(stderr, "\
Command denied. Valid debug level value is [0-3]\n");
                        return -1;
                    }
                }
                else if (debug.level < 1 || debug.level > 5) {
                    fprintf(stderr, "\
Command denied. Valid timing level value is [1-5]\n");
                    return -1;
                }
                break;

            case 'f':
                if (strstr(optarg,"/") && strstr(optarg,"\\")) {
                    fprintf(stderr, "Command denied. Invalid file name\n");
                    return -1;
                }
                fileOption = TRUE;
                memset(debug.logFileName,0,sizeof(debug.logFileName));
                ls_strcat(debug.logFileName,sizeof(debug.logFileName),optarg);
                if (debug.logFileName[strlen(debug.logFileName)-1] == '/' ||
                    debug.logFileName[strlen(debug.logFileName)-1] == '\\') {
                    fprintf(stderr, "\
Command denied. File name is needed after the path\n");
                return -1;
                }
                break;

            case 'o':
                debug.options = 1;
                break;

            default:
                return -2;
        }
    }
    if (checkPerm()) {
        if (opCode == LIM_DEBUG || opCode == LIM_TIMING ) {
            fprintf(stderr, "Operation permission denied by LIM\n");
        }
        else {
            fprintf(stderr, "Operation permission denied by RES\n");
        }
        return -1;
    }
    if (opCode == LIM_DEBUG || opCode == LIM_TIMING ) {
        if ( !fileOption ) {
            strcpy (debug.logFileName, "lim");
        }
        else {
            strcat(debug.logFileName,".lim");
        }
    }
    else {
        if ( !fileOption ) {
            strcpy (debug.logFileName, "res");
        }
        else {
            strcat(debug.logFileName,".res");
        }
    }
    if (optind >= argc) {
        if ((localHost = ls_getmyhostname())  == NULL) {
            ls_perror("ls_getmyhostname");
            return -1;
        }
        if (opCode == LIM_DEBUG || opCode == LIM_TIMING ) {
            limDebugReq (&debug, 1, &localHost);
        }
        else {
            resDebugReq (&debug, 1, &localHost);
        }
        return exitDebug;
    }

    if (optind == argc-1 && strcmp (argv[optind], "all") == 0) {

        hostInfo = ls_gethostinfo("-:server", &numHosts, NULL, 0, LOCAL_ONLY);
        if (hostInfo == NULL) {
            ls_perror("ls_gethostinfo");
            return -1;
        }
        for (i = 0; i<numHosts; i += 20) {
            num = (numHosts - i <  20) ? numHosts - i : 20;
            for (j = 0; j < num; j++)
                hosts[j] = hostInfo[i+j].hostName;

	    if (opCode == LIM_DEBUG || opCode == LIM_TIMING )
	        limDebugReq (&debug, num, hosts);
	    else
	        resDebugReq (&debug, num, hosts);
        }
        return exitDebug;
    }

    for (; optind < argc; optind += 20) {
        num = (argc - optind < 20) ? argc - optind : 20;
        if (opCode == LIM_DEBUG || opCode == LIM_TIMING )
            limDebugReq (&debug, num, &(argv[optind]));
        else
            resDebugReq (&debug, num, &(argv[optind]));
    }
    return exitDebug;
}

static int
checkPerm(void)
{
    struct clusterInfo *clusterInfo;
    char  *mycluster = NULL;
    int   nAdmins = -1;
    int i;
    char username[128];
    struct passwd *pw;

    if (getUser(username, sizeof(username)) < 0) {
        return -1;
    }

    if ((pw = getpwuid((uid_t)getuid())) != NULL) {
        if (pw->pw_uid == 0)
            return 0;
    }

    if ((mycluster = ls_getclustername()) == NULL) {
	ls_syslog(LOG_ERR, "%s: ls_getclustername failed %M", __func__);
	return RESE_DENIED;
    }
    if ((clusterInfo = ls_clusterinfo(NULL, NULL, NULL, 0, 0)) == NULL) {
	ls_syslog(LOG_ERR, "%s: ls_clusterinfo failed %M", __func__);
        return RESE_DENIED;
    }

    nAdmins = (clusterInfo->nAdmins) ? clusterInfo->nAdmins : 1;
    if (clusterInfo->nAdmins == 0) {
	if (strcmp(clusterInfo->managerName,username) == 0) {
	    return 0;
	}
    }
    for (i = 0; i < nAdmins; i++) {
        if (strcmp(clusterInfo->admins[i],username) == 0) {
            return 0;
	}
    }
    return -1;
}

static void
limDebugReq(struct debugReq *pdebug, int numHost, char *hosts[])
{
    int i;
    char strBuf[128];

    for (i=0; i<numHost; i++)
        if (hosts[i][0] != '\0') {
            if (oneLimDebug(pdebug, hosts[i]) == -1) {
		sprintf(strBuf, "Host %s set LIM debug failed",hosts[i]);
		ls_perror(strBuf);
		exitDebug = -1;
	    }
	    else {
		fprintf(stderr, "Host %s set LIM debug done\n", hosts[i]);
	    }
	}
    return ;

}

static void
resDebugReq(struct debugReq *pdebug, int numHost, char *hosts[])
{
    int pid;
    int i;
    LS_WAIT_T status;

    if ((pid = fork()) < 0) {
	perror("fork");
	exitDebug = -1;
	return ;
    }

    if (pid == 0) {

	if (ls_initrex(numHost, 0) < numHost ) {
	    ls_perror ("ls_initrex:" );
            exit (-1);
        }

        for (i = 0; i < numHost; i++) {
            if (ls_connect(hosts[i]) < 0) {
                ls_syslog(LOG_ERR, "Host %s set RES debug failed", hosts[i]);
	        exitDebug = -1;
                hosts[i][0] = '\0';
	    }
        }
        for (i = 0; i<numHost; i++) {
	    if (hosts[i][0] != '\0') {
	        if (oneResDebug (pdebug,hosts[i] ) == -1) {
		    ls_syslog(LOG_ERR, "Host %s set RES debug failed", hosts[i]);
		    exitDebug = -1;
                }
                printf("Host %s set RES debug done\n", hosts[i]);
            }
            exit(exitDebug);
        }
    }

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        exitDebug = -1;
        return ;
    }

    if (WIFEXITED(status) == 0) {
        fprintf(stderr, "Child process killed by signal.\n");
	exitDebug = -1;
	return;
    }

    if (WEXITSTATUS(status) == 0xff)
        exitDebug = -1;
}
