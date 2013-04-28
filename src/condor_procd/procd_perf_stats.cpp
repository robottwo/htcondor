
#include "condor_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "procd_perf_stats.h"

#ifdef HAVE_PERF_EVENT_H

using namespace condor;

ProcdPerfTracker * ProcdPerfTracker::m_instance = NULL;

ProcdPerfTracker &
ProcdPerfTracker::get_instance()
{
    if (m_instance == NULL)
    {
        m_instance = new ProcdPerfTracker();
    }
    return *m_instance;
}

int
ProcdPerfTracker::perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
     int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

int
ProcdPerfTracker::start_tracking(std::string & cgroup)
{
    int c_fd;

    // TODO: Open correct cgroup directory
    if ((c_fd = open("/cgroup/perf_event/foo", O_DIRECTORY)) == -1) {
        dprintf(D_ALWAYS, "Unable to open perf cgroup. (errno=%d, %s)\n", errno, strerror(errno));
        return -1;
    }

    // TODO: CPU counting anyone?
    int cpu_count = 4;
    int result_fds[METRIC_COUNT+1];
    result_fds[0] = c_fd;
    m_cgroup_fds.reserve(m_cgroup_fds.size()+(METRIC_COUNT+1)*cpu_count);
    for (int cpuid=0; cpuid<cpu_count; cpuid++)
    {
            int result = setup_cpu_tracking(c_fd, cpuid, PERF_FLAG_PID_CGROUP, result_fds+1);
            if (result < 0)
            {
                return result;
            }
            m_cgroups.push_back(cgroup);
            for (int idx=0; idx<METRIC_COUNT+1; idx++)
            {
                m_cgroup_fds.push_back(result_fds[idx]);
            }
            result_fds[0] = -1;
    }
    return 0;
}

int
ProcdPerfTracker::start_tracking(pid_t pid)
{
    int result_fds[METRIC_COUNT];
    int result = setup_cpu_tracking(pid, -1, 0, result_fds);
    if (result < 0) return result;

    m_pid_fds.reserve(m_pid_fds.size() + METRIC_COUNT);
    for (int idx=0; idx<METRIC_COUNT; idx++)
    {
        m_pid_fds.push_back(result_fds[idx]);
    }
    m_pids.push_back(pid);
    dprintf(D_ALWAYS, "Starting performance monitoring of PID %d (%ld processes monitored).\n", pid, m_pids.size());

    return 0;
}


int
ProcdPerfTracker::setup_cpu_tracking(int id, int cpuid, int flags, int result_fds[METRIC_COUNT])
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.read_format = PERF_FORMAT_GROUP;
    pe.mmap = 0;

    if ((result_fds[0] = perf_event_open(&pe, id, cpuid, -1, flags)) == -1)
    {
       dprintf(D_ALWAYS, "Error opening leader (cycles - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
       return -1;
    }

    pe.disabled = 0;
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    if ((result_fds[1] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        dprintf(D_ALWAYS, "Error opening group follower (instructions - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }

    pe.config = PERF_COUNT_HW_CACHE_REFERENCES;
    if ((result_fds[2] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        dprintf(D_ALWAYS, "Error opening group follower (cache references - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }

    pe.config = PERF_COUNT_HW_CACHE_MISSES;
    if ((result_fds[3] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        dprintf(D_ALWAYS, "Error opening group follower (cache misses - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }
    // Disabling these.  On KVM, it appears enabling these screws up other counters.
    // Will investigate later.
/*
    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_CPU_MIGRATIONS;
    if ((result_fds[4] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        fprintf(stderr, "Error opening group follower (CPU migrations - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }

    pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
    if ((result_fds[5] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        fprintf(stderr, "Error opening group follower (context switches - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }
*/
 /*   pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
    if ((result_fds[6] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        fprintf(stderr, "Error opening group follower (branch instructions - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }

    pe.config = PERF_COUNT_HW_BRANCH_MISSES;
    if ((result_fds[7] = perf_event_open(&pe, id, cpuid, result_fds[0], flags)) == -1)
    {
        fprintf(stderr, "Error opening group follower (branch misses - %llx) (errno=%d, %s)\n", pe.config, errno, strerror(errno));
        return -1;
    }
*/
    ioctl(result_fds[0], PERF_EVENT_IOC_RESET, 0);
    ioctl(result_fds[0], PERF_EVENT_IOC_ENABLE, 0);

    return 0;
}


int
ProcdPerfTracker::add_status(pid_t pid, ProcFamilyUsage &pfu, procInfo *info)
{
    unsigned idx=0;
    for (std::vector<int>::const_iterator it = m_pids.begin(); it != m_pids.end(); it++)
    {
        if (*it == pid)
        {
            dprintf(D_ALWAYS, "Retrieving perf stats for %d.\n", pid);
            return get_status_from_fds(&m_pid_fds[METRIC_COUNT*idx], pfu, info, true);
        }
        idx++;
    }
    return -1;
}


int
ProcdPerfTracker::get_status_from_fds(int fds[METRIC_COUNT], ProcFamilyUsage &pfu, procInfo *info, bool add)
{
    long long results[METRIC_COUNT+1];
    if (-1 == read(fds[0], results, (1+METRIC_COUNT)*sizeof(long long))) {
        dprintf(D_ALWAYS, "Error reading performance counters: %d, %s", errno, strerror(errno));
        return -1;
    }

    //if (results[0] != METRIC_COUNT) {
    if (results[0] != 4) {
        dprintf(D_ALWAYS, "Kernel returned the wrong number of events (%lld).\n", results[0]);
        return -1;
    }

    if (m_zero_next || !add)
    {
        m_zero_next = false;
        pfu.cpu_instructions = results[1];
        pfu.cpu_cycles = results[2];
        pfu.cpu_cache_references = results[3];
        pfu.cpu_cache_misses = results[4];
/*
        pfu.cpu_migrations = results[5];
        pfu.context_switches = results[6];
        pfu.cpu_branch_instructions = results[7];
        pfu.cpu_branch_misses = results[8];
*/
    }
    else
    {
        pfu.cpu_instructions += results[1];
        pfu.cpu_cycles += results[2];
        pfu.cpu_cache_references += results[3];
        pfu.cpu_cache_misses += results[4];
/*
        pfu.cpu_migrations += results[5];
        pfu.context_switches += results[6];
        pfu.cpu_branch_instructions += results[7];
        pfu.cpu_branch_misses += results[8];
*/
    }
    if (info)
    {
        info->cpu_instructions = results[1];
        info->cpu_cycles = results[2];
        info->cpu_cache_references = results[3];
        info->cpu_cache_misses = results[4];
/*
        info->cpu_migrations = results[5];
        info->context_switches = results[6];
        info->cpu_branch_instructions = results[7];
        info->cpu_branch_misses = results[8];
*/
    }
    return 0;
}


int
ProcdPerfTracker::stop_tracking(pid_t pid)
{
    std::vector<int>::iterator it2 = m_pid_fds.begin();
    dprintf(D_ALWAYS, "Removing PID %d from tracking.\n", pid);
    for (std::vector<pid_t>::iterator it = m_pids.begin(); it != m_pids.end() && it2 != m_pid_fds.end(); it++, it2+=METRIC_COUNT)
    {	
        if (*it == pid)
        {
            m_pid_fds.erase(it2, it2+METRIC_COUNT);
            return 0;
        }
    }
    return -1;
}


int
ProcdPerfTracker::print_status()
{
    std::vector<int>::iterator it2 = m_pid_fds.begin();
    for (std::vector<pid_t>::iterator it=m_pids.begin(); it != m_pids.end() && it2 != m_pid_fds.end(); it++,it2+=METRIC_COUNT)
    {
        printf("Performance status for %d:\n", *it);
        print_fd_status(&(*it2));
    }
    return 0;
}

int
ProcdPerfTracker::print_fd_status(int fds[METRIC_COUNT])
{
    ProcFamilyUsage pfu;
    m_zero_next = true;
    if (get_status_from_fds(fds, pfu, NULL, false) < 0)
    {
        printf("   ERROR: Unable to get status.\n");
        return -1;
    }
    printf("   CPU Instructions: %lld\n", pfu.cpu_instructions);
    printf("   CPU Cycles: %lld\n", pfu.cpu_cycles);
    printf("   CPU Cache References: %lld\n", pfu.cpu_cache_references);
    printf("   CPU Cache Misses: %lld\n", pfu.cpu_cache_misses);
    printf("   CPU Migrations: %lld\n", pfu.cpu_migrations);
    printf("   Context Switches: %lld\n", pfu.context_switches);
    printf("   CPU Branch Instructions: %lld\n", pfu.cpu_branch_instructions);
    printf("   CPU Branch Misses: %lld\n", pfu.cpu_branch_misses);

    return 0;
}

#endif
