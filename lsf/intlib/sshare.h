/*
 * Copyright (C) 2014-2016 David Bigagli
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */

#if !defined(_SSHARE_HEADER_)
#define _SSHARE_HEADER_

#include "tree.h"
#include "link.h"

#define SACCT_GROUP    0x1
#define SACCT_USER     0x2
#define SACCT_USER_ALL 0x4

/* The share account structure which is on the
 * share tree representing each user and group.
 */
struct share_acct {
    char *name;
    int uid;
    uint32_t shares;
    double dshares;
    uint32_t sent;
    int numPEND;
    int numRUN;
    int numRAN;
    int32_t dsrv2;
    uint32_t options;
};

/* Support data structure equivalent of groupInfoEnt
 * every change to groupInfoEnt must be added here.
 */
struct group_acct {
    char *group;
    char *memberList;
    char *user_shares;
    char *group_slots;
    int max_slots;
};

/* sshare_make_tree()
 *
 * Make the fairshare tree based on the MBD configuration
 * from lsb.queues and lsb.users
 *
 * The root of the tree is specified in lsb.queues:
 *
 *  USER_SHARES[[G1,1] [G2,1]]
 *
 * the users are specified in lsb.users and represented
 * as an array of group_acct (groupInfoEnt) data structures.
 *
 */
extern struct tree_ *sshare_make_tree(const char *,
                                      uint32_t,
                                      struct group_acct *);
extern struct share_acct *make_sacct(const char *, uint32_t);
extern void free_sacct(struct share_acct *);
extern int sshare_distribute_slots(struct tree_ *,
                                   uint32_t);
extern int sshare_distribute_own_slots(struct tree_ *,
				       uint32_t);

#endif /* _SSHARE_HEADER_ */