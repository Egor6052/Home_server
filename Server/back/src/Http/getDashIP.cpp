#include "Http.h"
#include <thread>
#include <random>

std::string Http::getDashIP()
{
    const std::string fallbackIp = "127.0.0.1";
    std::string resultIp = fallbackIp;

#ifdef _WIN32

   //#include <ifaddrs.h>
   // #include <arpa/inet.h>
    //#include <net/if.h>

    // Якщо в тебе вже є WSAStartup десь при старті програми — цей блок можна прибрати.
    WSADATA wsa{};
    const int wsaRes = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (wsaRes != 0)
        return fallbackIp;

    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG family = AF_INET;

    ULONG bufLen = 0;
    if (GetAdaptersAddresses(family, flags, nullptr, nullptr, &bufLen) != ERROR_BUFFER_OVERFLOW)
    {
        WSACleanup();
        return fallbackIp;
    }

    std::vector<unsigned char> buffer(bufLen);
    auto *addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());

    if (GetAdaptersAddresses(family, flags, nullptr, addrs, &bufLen) != NO_ERROR)
    {
        WSACleanup();
        return fallbackIp;
    }

    std::string bestPreferred; // Ethernet/Wi-Fi
    std::string bestAny;       // будь-який не-loopback

    for (auto *a = addrs; a != nullptr; a = a->Next)
    {
        // Бажано брати тільки "up"
        if (a->OperStatus != IfOperStatusUp)
            continue;

        const bool isPreferredType =
            (a->IfType == IF_TYPE_ETHERNET_CSMACD) || (a->IfType == IF_TYPE_IEEE80211);

        for (auto *ua = a->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
        {
            auto *sa = ua->Address.lpSockaddr;
            if (!sa || sa->sa_family != AF_INET)
                continue;

            auto *sin = reinterpret_cast<sockaddr_in *>(sa);
            if (is_loopback_ipv4(sin->sin_addr))
                continue;

            char buf[INET_ADDRSTRLEN]{};
            if (!InetNtopA(AF_INET, &sin->sin_addr, buf, sizeof(buf)))
                continue;

            if (bestAny.empty())
                bestAny = buf;
            if (isPreferredType && bestPreferred.empty())
                bestPreferred = buf;

            // Якщо вже знайшли пріоритетну адресу — можна не переривати одразу,
            // але зазвичай цього достатньо:
            if (!bestPreferred.empty())
                break;
        }

        if (!bestPreferred.empty())
            break;
    }

    if (!bestPreferred.empty())
        resultIp = bestPreferred;
    else if (!bestAny.empty())
        resultIp = bestAny;

    WSACleanup();
    return resultIp;

#else
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;

    if (getifaddrs(&interfaces) == 0 && interfaces)
    {
        temp_addr = interfaces;

        while (temp_addr != nullptr)
        {
            if (temp_addr->ifa_addr != nullptr && temp_addr->ifa_addr->sa_family == AF_INET)
            {
                std::string interfaceName = temp_addr->ifa_name;

                // loopback
                if (interfaceName != "lo")
                {
                    void *addr_ptr = &((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN]{};
                    if (inet_ntop(AF_INET, addr_ptr, addressBuffer, INET_ADDRSTRLEN))
                    {
                        resultIp = addressBuffer;

                        // Пріоритет як у тебе
                        if (interfaceName == "eth0" || interfaceName == "wlan0")
                        {
                            break;
                        }
                    }
                }
            }
            temp_addr = temp_addr->ifa_next;
        }

        freeifaddrs(interfaces);
    }

    return resultIp;
#endif

return resultIp;
}