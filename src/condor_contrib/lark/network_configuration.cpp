
#include <condor_common.h>
#include <condor_debug.h>

#include "network_configuration.h"
#include "nat_configuration.h"
#include "address_selection.h"
#include "local_address.h"

#include <classad/classad.h>

using namespace lark;

int
NetworkConfiguration::SelectAddresses()
{
	std::string address_type;
	if (!m_ad->EvaluateAttrString("LarkAddressType", address_type)) {
		address_type = "local";
	}
	if (address_type == "local") {
		m_address_selector = new HostLocalAddressSelection(m_ad);
	} else if (address_type == "dhcp") {
		m_address_selector = new DHCPAddressSelection(m_ad);
	} else if (address_type == "static") {
		m_address_selector = new StaticAddressSelection(m_ad);
	} else {
		dprintf(D_ALWAYS, "Unknown address selector type: %s\n", address_type.c_str());
		return 1;
	}
	return m_address_selector->SelectAddresses();
}

NetworkConfiguration *
NetworkConfiguration::GetNetworkConfiguration(classad_shared_ptr<classad::ClassAd> machine_ad)
{
	if (!machine_ad.get()) {
		return NULL;
	}
	std::string configuration_type;
	if (!machine_ad->EvaluateAttrString(ATTR_NETWORK_TYPE, configuration_type)) {
		configuration_type = "nat";
	}
	if (configuration_type == "nat") {
		return new NATConfiguration(machine_ad);
	} else {
		dprintf(D_ALWAYS, "Unknown configuration type \"%s\".\n", configuration_type.c_str());
	}
	return NULL;
}
