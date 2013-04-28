#ifndef __PROCD_PERF_STATS_H_
#define __PROCD_PERF_STATS_H_

#include "config.h"
#ifdef HAVE_PERF_EVENT_H

#include <vector>
#include <string>

#include "proc_family_io.h"

#define METRIC_COUNT 8

struct perf_event_attr;

namespace condor {

class ProcdPerfTracker
{
public:

    int start_tracking(std::string &cgroup);
    int stop_tracking(std::string &cgroup);

    int start_tracking(pid_t);
    int stop_tracking(pid_t);

    int get_status(std::string &cgroup, ProcFamilyUsage &usage);
    int add_status(pid_t pid, ProcFamilyUsage &usage, procInfo *info);
    void zero_usage() {m_zero_next = true;}

    int print_status();

    static ProcdPerfTracker &get_instance();

private:

    ProcdPerfTracker()
        : m_zero_next(true)
    {}

    int setup_tracking(int id, int flags);
    int setup_cpu_tracking(int id, int cpuid, int flags, int result_fds[METRIC_COUNT]);

    int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);

    int get_status_from_fds(int fds[METRIC_COUNT], ProcFamilyUsage &pfu, procInfo *info, bool);

    int print_fd_status(int fds[METRIC_COUNT]);

    std::vector<int> m_cgroup_fds;
    std::vector<std::string> m_cgroups;

    std::vector<int> m_pid_fds;
    std::vector<pid_t> m_pids;

    bool m_zero_next;

    static ProcdPerfTracker *m_instance;
};

}
#endif
#endif
