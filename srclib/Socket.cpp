#include "Socket.h"

int UDPSocket::JoinMulticast(const IPAddress & addr) {
	int level, optname;
	void *optval;
	socklen_t optlen;
    
    #ifdef __APPLE__
        	// NOTE: The ip_mreqn structure is not supported on Mac OS X,
        	// NOTE: so we use ip_mreq and ignore the interface parameter
		struct ip_mreq mreq4;
	#elif __unix__
		struct ip_mreqn mreq4;
	#endif
	
	struct ipv6_mreq mreq6;

	if (addr.GetType() == AF_INET) {
		addr.CopyBinaryData(&mreq4.imr_multiaddr.s_addr);
		
		#ifdef __APPLE__
			IPAddress::any4.CopyBinaryData(&mreq4.imr_multiaddr);	
		#elif __unix__
			IPAddress::any4.CopyBinaryData(&mreq4.imr_address);
			mreq4.imr_ifindex = 0;
		#endif
		
		level = IPPROTO_IP;
		optname = IP_ADD_MEMBERSHIP;
		optval = &mreq4;
		optlen = sizeof(mreq4);
	} else {
		addr.CopyBinaryData(&mreq6.ipv6mr_multiaddr);
		mreq6.ipv6mr_interface = 0;
		level = IPPROTO_IPV6;
		optname = IPV6_JOIN_GROUP;
		optval = &mreq6;
		optlen = sizeof(mreq6);
	}

	if (setsockopt(fd, level, optname, optval, optlen)) {
		RaiseError("UDPSocket::JoinMulticast");
		return errno;
	} else {
		return 0;
	}
}

