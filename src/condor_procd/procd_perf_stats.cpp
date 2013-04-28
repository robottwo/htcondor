
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

#include "../condor_starter.V6.1/cgroup.linux.h"
#include "procd_perf_stats.h"

int g_perf_restricted = 0;

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

#if defined(HAVE_EXT_LIBCGROUP)
int
ProcdPerfTracker::start_tracking(const std::string & cgroup)
{
    int c_fd;

    // Find the perf event controller
    void * handle = NULL;
    struct cgroup_mount_point mount_info;
    int ret = cgroup_get_controller_begin(&handle, &mount_info);
    std::stringstream perf_control;
    bool found_cg = false;
    while (ret == 0) {
        if (strcmp(mount_info.name, PERF_CONTROLLER_STR) == 0) {
            found_cg = true;
            perf_control << mount_info.path << "/";
            break;
        }
        cgroup_get_controller_next(&handle, &mount_info);
    }
    if (!found_cg && (ret != ECGEOF)) {
        dprintf(D_ALWAYS, "Error while locating perf event controller for procd: %u %s\n",
                        ret, cgroup_strerror(ret));
        return 1;
    }
    cgroup_get_controller_end(&handle);
    if (found_cg == false) {
        dprintf(D_ALWAYS, "Perf event controller is not available; will use per-PID tracking for performance data.\n");
        return 1;
    }

    // Finish constructing the location of the control files
    perf_control << cgroup;
    std::string perf_control_str = perf_control.str();
    if ((c_fd = open(perf_control_str.c_str(), O_DIRECTORY)) == -1) {
        dprintf(D_ALWAYS, "Unable to open perf cgroup. (errno=%d, %s)\n", errno, strerror(errno));
        return -1;
    }

    int result_fds[METRIC_COUNT+1];
    result_fds[0] = c_fd;
    // 16k cores should be enough for anyone...
    int cpu_count = m_cpu_count < 0 ? 16*1024 : m_cpu_count;
    m_cgroup_fds.reserve(m_cgroup_fds.size()+(METRIC_COUNT+1)*cpu_count);
    for (int cpuid=0; cpuid<cpu_count; cpuid++)
    {
        int result = setup_cpu_tracking(c_fd, cpuid, PERF_FLAG_PID_CGROUP, result_fds+1);
        if (result == -EINVAL)
        {
            m_cpu_count = cpuid-1;
            break;
        }
        else if (result < 0)
        {
            dprintf(D_ALWAYS, "Failed to setup CPU tracking for CPU %d.\n", cpuid);
            return result;
        }
        m_cgroups.push_back(cgroup);
        for (int idx=0; idx<METRIC_COUNT+1; idx++)
        {
            m_cgroup_fds.push_back(result_fds[idx]);
        }
        result_fds[0] = -1;
    }
    dprintf(D_ALWAYS, "Starting performance monitoring of cgroup %s.\n", cgroup.c_str());
    return 0;
}
#endif

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
        if (errno == EINVAL) return -EINVAL;
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
    // Selectively disable these metrics.  On KVM, it appears enabling them screws up other counters.
    
    if (!g_perf_restricted)
    {
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

        pe.type = PERF_TYPE_HARDWARE;
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
    }
    else
    {
        result_fds[4] = -1;
        result_fds[5] = -1;
        result_fds[6] = -1;
        result_fds[7] = -1;
    }
    ioctl(result_fds[0], PERF_EVENT_IOC_RESET, 0);
    ioctl(result_fds[0], PERF_EVENT_IOC_ENABLE, 0);

    return 0;
}


#if defined(HAVE_EXT_LIBCGROUP)
int
ProcdPerfTracker::get_status(const std::string &cgroup, ProcFamilyUsage &pfu)
{
    unsigned idx=0;
    int ret;
    for (std::vector<std::string>::const_iterator it = m_cgroups.begin(); it != m_cgroups.end(); it++, idx++)
    {
        if (*it == cgroup)
        {
            if ((ret = get_status_from_fds(&m_cgroup_fds[(1+METRIC_COUNT)*idx+1], pfu, NULL, true)) < 0)
            {
                return ret;
            }
        }
    }
    return 0;
}
#endif

int
ProcdPerfTracker::add_status(pid_t pid, ProcFamilyUsage &pfu, procInfo *info)
{
    unsigned idx=0;
    zero_usage();
    for (std::vector<int>::const_iterator it = m_pids.begin(); it != m_pids.end(); it++, idx++)
    {
        if (*it == pid)
        {
            dprintf(D_PROCFAMILY, "Retrieving perf stats for %d.\n", pid);
            return get_status_from_fds(&m_pid_fds[(METRIC_COUNT)*idx], pfu, info, true);
        }
    }
    return -1;
}


int
ProcdPerfTracker::get_status_from_fds(int fds[METRIC_COUNT], ProcFamilyUsage &pfu, procInfo *info, bool add)
{
    long long results[METRIC_COUNT+1];
    if (-1 == read(fds[0], results, (1+METRIC_COUNT)*sizeof(long long))) {
        dprintf(D_ALWAYS, "Error reading performance counters (fd %d): %d, %s\n", fds[0], errno, strerror(errno));
        return -1;
    }

    if ((g_perf_restricted && (results[0] != 4)) || (!g_perf_restricted && (results[0] != METRIC_COUNT))) {
        dprintf(D_ALWAYS, "Kernel returned the wrong number of events (%lld).\n", results[0]);
        return -1;
    }

    //dprintf(D_ALWAYS, "Instructions - %lld, Cycles - %lld\n", results[1], results[2]);
    if (m_zero_next || !add)
    {
        m_zero_next = false;
        pfu.cpu_instructions = results[1];
        pfu.cpu_cycles = results[2];
        pfu.cpu_cache_references = results[3];
        pfu.cpu_cache_misses = results[4];
        if (!g_perf_restricted)
        {
            pfu.cpu_migrations = results[5];
            pfu.context_switches = results[6];
            pfu.cpu_branch_instructions = results[7];
            pfu.cpu_branch_misses = results[8];
        }
        else
        {
            pfu.cpu_migrations = -1;
            pfu.context_switches = -1;
            pfu.cpu_branch_instructions = -1;
            pfu.cpu_branch_misses = -1;
        }
    }
    else
    {
        pfu.cpu_instructions += results[1];
        pfu.cpu_cycles += results[2];
        pfu.cpu_cache_references += results[3];
        pfu.cpu_cache_misses += results[4];
        if (!g_perf_restricted)
        {
            pfu.cpu_migrations += results[5];
            pfu.context_switches += results[6];
            pfu.cpu_branch_instructions += results[7];
            pfu.cpu_branch_misses += results[8];
        }
    }
    if (info)
    {
        info->cpu_instructions = results[1];
        info->cpu_cycles = results[2];
        info->cpu_cache_references = results[3];
        info->cpu_cache_misses = results[4];
        if (!g_perf_restricted)
        {
            info->cpu_migrations = results[5];
            info->context_switches = results[6];
            info->cpu_branch_instructions = results[7];
            info->cpu_branch_misses = results[8];
        }
        else
        {
            info->cpu_migrations = -1;
            info->context_switches = -1;
            info->cpu_branch_instructions = -1;
            info->cpu_branch_misses = -1;
        }
    }
    return 0;
}


int
ProcdPerfTracker::stop_tracking(pid_t pid)
{
    std::vector<int>::iterator it2 = m_pid_fds.begin();
    std::vector<pid_t>::iterator it = m_pids.begin();
    dprintf(D_ALWAYS, "Removing PID %d from tracking.\n", pid);
    while (it != m_pids.end() && it2 != m_pid_fds.end())
    {	
        if (*it == pid)
        {
            for (unsigned idx=0; idx<METRIC_COUNT; idx++)
            {
                if (*(it2+idx) >= 0) close(*(it2+idx));
            }
            it2 = m_pid_fds.erase(it2, it2+METRIC_COUNT);
            it = m_pids.erase(it);
            return 0;
        }
        else
        {
            it++;
            it2+=METRIC_COUNT;
        }
    }
    return -1;
}


#if defined(HAVE_EXT_LIBCGROUP)
int
ProcdPerfTracker::stop_tracking(const std::string &cgroup)
{
    unsigned count = 0;
    std::vector<int>::iterator it2 = m_cgroup_fds.begin();
    std::vector<std::string>::iterator it = m_cgroups.begin();
    dprintf(D_ALWAYS, "Removing cgroup %s from tracking.\n", cgroup.c_str());
    while (it != m_cgroups.end() && it2 != m_cgroup_fds.end())
    {
        if (*it == cgroup)
        {
            for (unsigned idx=0; idx<METRIC_COUNT+1; idx++)
            {
                if (*(it2+idx) >= 0) close(*(it2+idx));
            }
            it2 = m_cgroup_fds.erase(it2, it2+METRIC_COUNT+1);
            it = m_cgroups.erase(it);
            count++;
        }
        else
        {
            it++;
            it2+=METRIC_COUNT+1;
        }
    }
    if (count) return 0;
    return -1;
}
#endif


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
