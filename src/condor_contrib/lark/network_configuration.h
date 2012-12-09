
/*
 * The NetworkConfiguration base class is used when creating
 * and destroying network configurations.
 *
 * This is an abstract class and primarily provides an interface for
 * others to fill in.
 *
 */

#ifndef __network_configuration_h_
#define __network_configuration_h_

#define ATTR_IPTABLE_NAME "LarkIptableName"
#define ATTR_NETWORK_TYPE "LarkNetworkType"
#define ATTR_INTERNAL_INTERFACE "LarkInternalInterface"
#define ATTR_EXTERNAL_INTERFACE "LarkExternalInterface"

#include <classad/classad_stl.h>

namespace classad {
	class ClassAd;
}

namespace lark {

class AddressSelection;

class NetworkConfiguration {

public:
	/*
	 * The constructor is given a shared reference to the machine ad.
	 *
	 * The object may add relevant network attributes during setup or cleanup.
	 * Attributes prefixed with "Lark" may be forwarded to the schedd.
	 */
	NetworkConfiguration(classad_shared_ptr<classad::ClassAd> machine_ad)
		: m_address_selector(NULL),
		  m_ad(machine_ad)
		{}

	/*
	 * Select an address for the job-specific internal and external device.
	 *
	 * Results should be placed in the machine ClassAd using the following attributes:
	 * - LarkInnerAddressIPv4: the IPv4 address in the internal network namespace.
	 * - LarkInnerAddressIPv6: the IPv6 address in the internal network namespace.  If left
	 *   blank, SLAAC may be used.
	 * - LarkOuterAddressIPv4: the IPV4 address in the external network namespace.
	 * - LarkOuterAddressIPv6: the IPv6 address in the external network namespace.  If left
	 *   blank, SLAAC may be used.
	 *
	 *  Note this should be configuration-only: the NetworkNamespaceManager will
	 *  look at the results and assign addresses at the appropriate time.
	 *
	 * The following useful attributes may be present:
	 * - LarkInnerDevice - the network device seen by the job.
	 * - LarkOuterDevice - the job-specific network device as seen by the system.
	 *
	 * Return 0 on success and -1 otherwise.
	 */
	virtual int SelectAddresses();

	/*
	 * Setup the network configuration.
	 * This will be invoked after a successful SelectAddresses.
	 *
	 * Unlike SelectAddresses, this should perform changes to the kernel as
	 * necessary.
	 *
	 * This will be invoked by the NetworkNamespaceManager in PreFork.
	 */
	virtual int Setup() = 0;

	/*
	 * Cleanup any kernel data structures which are not cleaned up automatically.
	 */
	virtual int Cleanup() = 0;

	/*
	 * Returns a pointer (possibly null) to a network configuration object.
	 */
	static NetworkConfiguration * GetNetworkConfiguration(classad_shared_ptr<classad::ClassAd> machine_ad);

private:
	AddressSelection *m_address_selector;
	classad_shared_ptr<classad::ClassAd> m_ad;
};

}

#endif
