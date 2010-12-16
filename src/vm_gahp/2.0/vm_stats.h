/***************************************************************
 *
 * Copyright (C) 1990-2010, Redhat.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#ifndef VMGAHP_STATS
#define VMGAHP_STATS

namespace condor
{

    namespace vmu
    {

    /**
     *  The following are common
     */

    /// The state of the current running vm
    typedef enum
    {
        VM_INACTIVE=0,
        VM_RUNNING,
        VM_STOPPED,
        VM_SUSPENDED
        // may be more...
    }vm_state;

    /// vm_stats are all the current known information about a vm for
    typedef struct stats
    {
        //enum type eVmType;
        vm_state        m_eState; ///<
        unsigned int    m_iPid;   ///< pid of currently running vm 0

        // insert all the vm stats here.
        //// running stats
        //// static config.
        //// list of checkpoints, or just last checkpoint?
    }vm_stats;


    } // namespace vmu

}// namespace condor

#endif
