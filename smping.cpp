

#include "smping.h"

#include <winsock2.h>
// #include <winsock.h>    //  Complier force winsock2.h set before winsock.h
#include <ws2tcpip.h>
#include <windns.h>
#include <winerror.h>
#include <iphlpapi.h>
#include <windows.h>    // Complier recommand to put windows.h after winsock2.h

// #pragma comment (lib, "iphlpapi.lib")
// #pragma comment (lib, "ws2_32.lib")    // useless with gcc on windows
// to declare to use static library, add it to the gcc args (in .vscode/tasks.json)

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <iomanip>
#include <string>

// #define RAND_MAX 126-32    // Number of displayable characters in ASCII

// !!! Must put after #include "smping.h"
// May speed up in a certain degree, I guess :(
const int IcmpHeaderSize = sizeof(IcmpHeader);

#define ICMP_HEADER_SIZE IcmpHeaderSize
#define RECV_BUFFER_SIZE 1024

// Customed malloc() and free()
// See https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getnetworkparams
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

using namespace std;

int main(int argc, char* argv[]){
    try{
        ParseResult* pParseRes = checkArgs(argc, argv);
        PingInfo* pPingRes = NULL;

        if (pParseRes->debug){
            cerr << "argc: " << argc << endl;
            for(int i=0; i<argc; i++)
                cerr << "argv[" << i << "] " << argv[i] << endl;

        if (pParseRes->loop)
            pParseRes->count = numeric_limits<int>::max();

            cerr << endl;
            cerr << "Parameters resolve result:" << endl
                 << "  target.isIPAddr = " << bool2Char(pParseRes->isIPAddr) << endl
                 << "  Loop = " << bool2Char(pParseRes->loop) << endl
                 << "  Debug enabled = " << bool2Char(pParseRes->debug) << endl
                 << "  Target in parameter = " << pParseRes->target << endl
                 << "  IP address of target = " << pParseRes->ip << endl
                 << "  Test count = " << pParseRes->count << endl
                 << "  ICMP message size = " << pParseRes->size << endl;
        }

        if (!(pParseRes->isIPAddr))
            pParseRes->ip = string(NsLookup(pParseRes->target, pParseRes->debug, cerr));
        else
            pParseRes->ip = pParseRes->target;

        cout << "PING " << pParseRes->target << " (" << pParseRes->ip << ") "
             << pParseRes->size << '(' << (pParseRes->size)+28 << ')'
             << " bytes of data:"
             << endl;

        pPingRes = Ping(pParseRes->ip, pParseRes->loop, pParseRes->count, pParseRes->size, 1, pParseRes->debug, cout, cerr);

        cout << endl;
        cout << "-- " << pParseRes->ip << " ping statistics --" << endl
             << pParseRes->count << " packets transmitted, " 
             << pParseRes->count - pPingRes->lost << " received, "
             << fixed << setprecision(2)
             << (float)((pPingRes->lost)/(pParseRes->count)) << "% packet loss, "
            //  << "time " << pPingRes->sumTime ""
            << endl;
        cout << "RTT min/avg/max = " 
             << fixed << setprecision(3)
             << pPingRes->minRTT << " / "
             << pPingRes->maxRTT << " / "
             << (double)((pPingRes->sumTime)/(pParseRes->count))
             << " ms" << endl << endl;
    }
    catch(ParaResolveFailedException& e){
        cerr << e.what() <<endl;
    }

    return 0;
}

// Convert bool to const char*, to make debug infomation more beautiful
inline const char* bool2Char(bool input){
    return input ? "true" : "false" ;
}

// Check the args attached to smping.exe
ParseResult* checkArgs(int argc, char* argv[]){
    ParseResult* pRes = new ParseResult;

    if (argc == 1)     // If not give parameter 
        throw ParaResolveFailedException();
    else {
        pRes->target.assign(argv[argc-1]);

        for (int i=1; i <= (argc-2); i++){
            // cout << "checkArgs(): " << endl;
            // cout << "argv[" << i << "] " << argv[i] <<endl;
            char* current = argv[i];
            // Accpet - / \ to specified parameters
            if (current[0] == '-' || current[0] == '/' || current[0] == '\\')
                if (current[1] == 't')
                    pRes->loop = true;

                else if ((string(current)).find("debug") != string::npos)
                    pRes->debug = true;

                else if (current[1] == 'l' || current[1] == 'n'){
                    for (int j=0; j<sizeof(argv[i+1])-1; j++)
                        if (argv[i+1][j] < '0' || argv[i+1][j] > '9')
                            if (argv[i+1][j] != '\0')
                                throw ParaResolveFailedException();

                    if (argv[i][1] == 'l')
                    pRes->size = atoi(argv[++i]);
                    else
                        if(pRes->loop == false)
                            pRes->count = atoi(argv[++i]);
                        else
                            throw ParaResolveFailedException();
                }

        }
    }
    // Count the '.' appeared in target, to check whether IP address or hostname
    const char* temp = (pRes->target).c_str();
    int dotCount = 0;
    for (int i=0; i<(pRes->target).size(); i++){
        if (temp[i] < '0' || temp[i] > '9'){
            if (temp[i] == '.')
                dotCount++;
            else
                pRes->isIPAddr = false;
        }
    }
    if (dotCount != 3 && pRes->isIPAddr == true) throw ParaResolveFailedException();
    if (pRes->count == 0 && pRes->loop == false) pRes->count = 4;
    return pRes;
}

// resolve the input hostname to IP address, use gethostbyname() in Winsock library
char* NsLookup(string& hostname, bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "NsLookup() called" << endl;

    // If use string to declare ip then return it's reference, the Program will exit unexpectly
    char* ip = new char[24];
    try{
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        hostent* host = gethostbyname(hostname.c_str());

        if (host == NULL){
            if (debug){
                errOut << "  Winsock gethostbyname() failed.";
                int dwError = WSAGetLastError();
                if (dwError != 0) {
                    switch (dwError){
                        case WSAHOST_NOT_FOUND:
                            errOut << "Host not found." << endl;
                            break;
                        case WSANO_DATA:
                            errOut << "No data record found." << endl;
                            break;
                        default:
                            errOut << "Error code:" << dwError << endl;
                            break;
                    }
                }
                errOut << "  Try NsLookupFull() to get target IP." << endl;
            }
            strcpy(ip, NsLookupFull(hostname, debug, errOut));
        }
        else
            ip = inet_ntoa(*(in_addr*)host->h_addr);

        if (debug)
            errOut << "  Success.IP resolved: " << ip << endl << endl;
    }
    catch (exception& e){
    }
    return ip;
}

// Get current DNS configurations, using GetNetworkParams()
DNSList* GetDNSList(bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << "  GetDNSList() called" << endl;
    try{
        FIXED_INFO *pFixedInfo;
        ULONG ulOutBufLen;
        DWORD dwRetVal;
        IP_ADDR_STRING *pIPAddr;

        pFixedInfo = (FIXED_INFO *)MALLOC(sizeof(FIXED_INFO));
        ulOutBufLen = sizeof(FIXED_INFO);

        // Call GetAdaptersInfo() once to get the correct value of ulOutBufLen
        if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            FREE(pFixedInfo);
            pFixedInfo = (FIXED_INFO *) MALLOC(ulOutBufLen);
            if (pFixedInfo == NULL) 
                // The meaning of errcode can see in smping.h
                throw GetNetworkParamsFailedException(1);
        }

        dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen);
        if (dwRetVal == NO_ERROR){
            pIPAddr = &(pFixedInfo->DnsServerList);

            DNSList* head = new DNSList;
            head->ip = pIPAddr->IpAddress.String;

            if (debug)
                errOut << "    GetNetworkParams() successed." << endl
                       << "    DNS Server obtained:" << endl;

            DNSList* temp = head;
            while(pIPAddr != NULL){
                if (debug)
                    errOut << "      " << pIPAddr->IpAddress.String << endl;
                temp->next = new DNSList;
                temp = temp->next;
                // Allocate a piece of memory for DNSList to avoid bug when return it
                temp->ip = (char*)malloc(sizeof(pIPAddr->IpAddress.String));
                memcpy(temp->ip, pIPAddr->IpAddress.String, sizeof(pIPAddr->IpAddress.String));
                pIPAddr = pIPAddr->Next;
            }

            return head;
        }else
            throw GetNetworkParamsFailedException(2, dwRetVal);
    }
    catch (exception& e){
        errOut << e.what();
    }
    return NULL;
}

// Use DNSAPI by Windows to launch a DNS query
// See also https://docs.microsoft.com/en-us/troubleshoot/windows/win32/use-dnsquery-resolve-host-names
const char* NsLookupFull(string& hostname, bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "NsLookupFull() called" << endl;
    try{
        char* pOwnerName = (char*)malloc(hostname.size() * sizeof(char) + 1);
        strcpy(pOwnerName, hostname.c_str());

        WORD wType = DNS_TYPE_A;
        PIP4_ARRAY pSrvList = (PIP4_ARRAY)LocalAlloc(LPTR, sizeof(IP4_ARRAY)); //Pointer to IP4_ARRAY structure.
        DNS_STATUS status; //Return value of DnsQuery_A() function.
        PDNS_RECORD pDnsRecord; //Pointer to DNS_RECORD structure.
        DNS_FREE_TYPE freetype;
        freetype = DnsFreeRecordList;

        DNSList* pDnsList = GetDNSList(debug, errOut);
        for(int i=1; pDnsList!=NULL; i++){
            pSrvList->AddrCount = i;
            pSrvList->AddrArray[0] = inet_addr(pDnsList->ip);
            pDnsList = pDnsList->next;
        }

        // Strangely, the VSCode C++ Lint claimed that the first parameter of DnsQuery() is PCWSTR
        // But gcc think it should be PCSTR and refuse to compile with PCWSTR inputed
        status = DnsQuery(pOwnerName, wType, DNS_QUERY_BYPASS_CACHE, pSrvList, &pDnsRecord, NULL);

        if (status)
            throw WinDnsApiQueryFailedException(status);
        else
            return (const char*)pDnsRecord->Data.PTR.pNameHost;

        DnsRecordListFree(pDnsRecord, freetype);
        LocalFree(pSrvList);
    }
    catch (exception& e){
        errOut << e.what();
    }
    return NULL;
}

// Ping destnation IP, default to 32 Byte data pack and repeat for 4 times
PingInfo* Ping(string& destIP, bool loop = false, int count = 4, int size = 32, int seqStart = 1, bool debug = false, ostream& stdOut = cout, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "Ping() called" << endl;

    PingInfo* pRes = new PingInfo;

    try{
        int msTimeOut = 1000;    // Max timeout: 1s
        char sendBuff[ICMP_HEADER_SIZE + size];

        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&msTimeOut, sizeof(msTimeOut));

        sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_addr.S_un.S_addr = inet_addr(destIP.c_str());
        dest.sin_port = htons(0);

        IcmpHeader* pIcmpReqHdr = (IcmpHeader*)sendBuff;    // set icmp Head
        pIcmpReqHdr->type = 0x08;    // ICMP Type: request echo
        pIcmpReqHdr->code = 0;
        pIcmpReqHdr->id = (USHORT)::GetCurrentProcessId();    //ICMP id: usually used to identify which process send icmp request
        // pIcmpReqHdr->seq = seqStart;
        pIcmpReqHdr->CheckSum = 0;

        // Fullfill the data part in ICMP package
        memset((char*)(sendBuff+ICMP_HEADER_SIZE), 'a', size);

        sockaddr_in recvAddr;
        int recvLen = sizeof(recvAddr);
        char recvBuff[RECV_BUFFER_SIZE];
        int sendStatus, recvStatus, sendErrorCode;
        bool isTimeOut = false;

        // High accuracy timer
        LARGE_INTEGER startClock, endClock;
        LARGE_INTEGER timerFreq;

        QueryPerformanceFrequency(&timerFreq);
        double quadpart = timerFreq.QuadPart;

        QueryPerformanceCounter(&startClock);

        // Cycled pinging target
        for (int times=0; times<count; times++){
            // Add sequence no. and checksum to ICMP header
            pIcmpReqHdr->seq = seqStart + times;
            pIcmpReqHdr->CheckSum = CheckSum(pIcmpReqHdr, sizeof(sendBuff));

            sendStatus = sendto(sock, sendBuff, sizeof(sendBuff), 0, (SOCKADDR*)&dest, sizeof(sendBuff));
            sendErrorCode = WSAGetLastError();

            int resps = 0;    // Number of response

            int i = 0;
            while(true){
                if(i++ > 5){    // Try 5 times to receive reply message but timeout
                    isTimeOut = true;
                    break;
                }

                memset(recvBuff, 0, RECV_BUFFER_SIZE);    // Set the receive buffer to full zero

                int recvStatus = recvfrom(sock, recvBuff, MAXBYTE, 0, (SOCKADDR*)&recvAddr, &recvLen);

                if(!strcmp(inet_ntoa(recvAddr.sin_addr), destIP.c_str())){
                    resps++;
                    break;
                }
            }
            QueryPerformanceCounter(&endClock);

            // Set debug log after timer to avoid extra delay
            if (debug)
                errOut << "  Send info: " << "status: " << sendStatus 
                       << " Error code: " << sendErrorCode
                       << endl
                       << "  Receive info: " << "status: " << recvStatus 
                       << " Error code: " << WSAGetLastError()
                       << endl;

            if(isTimeOut)
                throw WinsockRecvTimeOutException();

            char ipInfo = recvBuff[0];    // First 8 bits for IP version and head length
            short* ipMsgLen = (short*)&(recvBuff[2]);    // 3~4 bytes for total length of IP message
            char recvTTL = recvBuff[8];    // 9th byte for ttl

            int ipVer = ipInfo >> 4;
            int ipHeadLen = ((ipInfo << 4) >> 4) * 4;

            // The ICMP Message just after IP head Message, use ipHeadLen to locate it
            IcmpHeader* pIcmpResp = (IcmpHeader*)(recvBuff + ipHeadLen);

            // PingInfo* result = new PingInfo;

            if(pIcmpResp->type == 0){    // ICMP echo reply message
                double durTime = (endClock.QuadPart - startClock.QuadPart) / quadpart;
                // result->seq = seqStart;
                // result->len = ntohs(*ipMsgLen);
                // result->ttl = recvTTL;
                int checksum = ntohs(pIcmpResp->CheckSum);

                stdOut << *ipMsgLen << " bytes from " << destIP << ':'
                    << " ICMP_sequence=" << pIcmpReqHdr->seq
                    << " TTL=" << int(recvTTL)
                    << " Time=" <<fixed << setprecision(3) << durTime << " ms";

                if (debug)
                    errOut << " CheckSum=" << checksum << endl;
                else
                    stdOut << endl;

                pRes->sumTime += durTime;

                if (pRes->minRTT > durTime)
                    pRes->minRTT = durTime;

                if (pRes->maxRTT < durTime)
                    pRes->maxRTT = durTime;

            }else{
                // throw IcmpRecvFailedException();
                errOut << "Timeout." << endl;
                pRes->lost += 1;
            }

        Sleep(500);
        }

        closesocket(sock);
        return pRes;

    }catch(WinsockRecvTimeOutException& e){
        errOut << e.what();
    }catch(IcmpRecvFailedException& e){
        errOut << e.what();
    }
    return NULL;
}

// Calculate the checksum from ICMP header data
unsigned short CheckSum(IcmpHeader* head, int len){
    unsigned short *temp = (unsigned short *)head;
    unsigned int sum = 0x0;

    while(len > 1){
        sum += *(temp++);
        len -= 2;
    }

    if(len)    // If len%2 == 1
        sum += *((unsigned short*)head);

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >>16);

    return (unsigned short)(~sum);
}

/* 
 * References:
 * https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-setsockopt
 * http://c.biancheng.net/cpp/socket/
 * https://github.com/licongxing/myping/blob/master/main.c
 * https://blog.csdn.net/u013183287/article/details/99622725
 */