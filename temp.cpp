//这个文件用来存一些我不舍得扔掉的代码
#include "smping.h"

#include <winsock2.h>    //  Complier force winsock2.h set before winsock.h
// #include <winsock.h>
#include <ws2tcpip.h>
#include <windns.h>
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

const int DnsMsgSize = sizeof(dnsMessage);
const int DnsQueryInfoSize = sizeof(dnsQueryInfo);
const int IcmpHeaderSize = sizeof(icmpHeader);

// Customed malloc() and free(); See https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getnetworkparams
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#define DNS_MESSAGE_SIZE DnsMsgSize
#define DNS_QUERY_INFO_SIZE DnsQueryInfoSize
#define ICMP_HEADER_SIZE IcmpHeaderSize
#define RECV_BUFFER_SIZE 1024
#define LOCAL_PORT 56789


using namespace std;

int main(){
    icmpHeader* test = new icmpHeader;
    test->checkSum = 0;
    test->code = 0;
    test->type = 0x08;
    test->seq = 1;
}

short checkSum2(icmpHeader* head, int len){
    unsigned short *temp = (unsigned short *)head;
    int sum = 0;

    while(len > 1){
        sum += *(temp++);
        if(sum & 0x10000)    // To check if the highest bit carried
            sum = (sum & 0xffff) + 1;
        len -= 2;
    }

    if(len){    // If len%2 == 1
        sum += ((unsigned short) *((unsigned char*)temp) << 8);
        if(sum & 0x10000)
            sum = (sum & 0xffff) + 1;
    }

    return (sum & 0xffff);
}

parseResult* checkArgs(int argc, char* argv[]){
    parseResult* res = new parseResult;

    // assign the hostname/ip to res->target
    if (argc == 1)
        if (argv[0] == "--debug")
            res->target = "223.5.5.5";
        else
            res->target.assign(argv[0]);
    else{
        res->target.assign(argv[argc-1]);

        for (int i=0; i<(argc-1); i++)

            if (argv[i] == "--debug"){
                res->ip = "223.5.5.5";
                return res;
            }

            else if (argv[i] == "-t" || argv[i] == "/t")
                res->loop = true;

            else if (argv[i] == "-l" || argv[i] == "-n"){
                for (int j=0; j<sizeof(argv[i+1]); j++)
                    if((argv[i+1][j] < '0') || (argv[i+1][j] > '9'))    // If not in 0~9
                        throw ParaResolveFailedException();
                    else continue;
                if (argv[i] == "-l")
                    res->size = atoi(argv[i++]);
                else if (argv[i] == "-n")
                    res->count = atoi(argv[i++]);
            }
    }

    // check if target is IP Address
    // ASCII 46:. 48~57:0~9
    string::iterator iter = res->target.begin();
    int dotCount = 0;
    for(; iter != res->target.end(); iter++)
        if((*iter == '.') || ((*iter >= '0') && (*iter <= '9')))
            if(*iter == '.')
                dotCount++;
            else
                continue;
        else
            res->isIPAddr = false;

    if(dotCount > 3)
        throw ParaResolveFailedException();

    res->isIPAddr = true;

    return res;
}


unsigned short chsum(icmpHeader *picmp, int len) {
    long sum = 0;
    unsigned short *temp = (unsigned short *)picmp;

    while (len > 1){
        sum += *(temp++);
        if ( sum & 0x80000000 )
            sum = (sum & 0xffff) + (sum >> 16);
        len -= 2;
    }

    if (len)
        sum += (unsigned short)*(unsigned char *)temp;

    while ( sum >> 16 )
        sum = (sum & 0xffff) + (sum >> 16);

    return (unsigned short)~sum;
}

// checksum() from Microsoft offical example
USHORT checksum(USHORT *buffer, int size) 
{
    unsigned long cksum=0;

    while (size > 1) 
    {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size) 
    {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (USHORT)(~cksum);
}
// Get current DNS configurations, using GetNetworkParams()
DNSList* getDNSList(bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "getDNSList() called" << endl;
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
                errOut << endl << "GetNetworkParams() successed." << endl
                       << "DNS Server obtained:" << endl;

            DNSList* temp = head;
            while(pIPAddr != NULL){
                if (debug)
                    errOut << '\t' << pIPAddr->IpAddress.String << endl;
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
    catch (exception& e){}
    return NULL;
}

// Send DNS query message to get target IP, will be called if nslookup() failed
char* nslookupFull(string& hostname, bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "nslookupFull() called" << endl;

    char* ip = NULL;
    try{
        DNSList* pDnsList = getDNSList(debug, errOut);

        const char* hostName = hostname.c_str();
        int targetLen = sizeof(hostName) + 1;
        char* targetName = (char*)malloc(targetLen);
        memcpy(targetName+sizeof(char), hostName, targetLen-1);

        // Organize domin name in DNS message format
        for(int i=targetLen-1; i>=0; i--){
            int len = 0;
            targetName[targetLen] = 0;

            if (targetName[i] != '.')
                len++;
            else{
                targetName[i] = len;
                len = 0;
            }
        }

        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        bool flag = false;

        for(int i=1; flag; i++){
            sockaddr_in sendSockAddr;
            sendSockAddr.sin_family = AF_INET;
            sendSockAddr.sin_addr.S_un.S_addr = inet_addr(pDnsList->ip);
            sendSockAddr.sin_port = htons(53);

            int sizeOfSendBuffer = DNS_MESSAGE_SIZE + targetLen + DNS_QUERY_INFO_SIZE;
            char sendBuff[sizeOfSendBuffer];

            dnsMessage* pDnsQueryMsg = (dnsMessage*)sendBuff;
            pDnsQueryMsg->id = i;
            pDnsQueryMsg->flag = 0x0100;    //Standard query
            pDnsQueryMsg->questions = 1;
            pDnsQueryMsg->ansRR = 0;
            pDnsQueryMsg->authRR = 0;
            pDnsQueryMsg->adlRR = 0;

            // Target name in DNS message format, after basic sturct part
            memcpy(sendBuff+DNS_MESSAGE_SIZE, targetName, targetLen);

            dnsQueryInfo* pDnsQueryInfo = (dnsQueryInfo*)(sendBuff + DNS_MESSAGE_SIZE + targetLen);
            pDnsQueryInfo->type = 0x0001;    //Type: A(Host Address)
            pDnsQueryInfo->dnsClass = 0x0001;    //Class: IN

            sockaddr_in recvSockAddr;
            // recvSockAddr.sin_addr.S_un.S_addr = inet_addr(pDnsList->ip);
            // recvSockAddr.sin_family = AF_INET;

            int sizeOfRecvSockAddr = sizeof(recvSockAddr);
            char recvBuff[RECV_BUFFER_SIZE];
            int sendStatus, recvStatus, sendErrorCode;

            sendStatus = sendto(sock, sendBuff, sizeOfSendBuffer, 0, (SOCKADDR*)pDnsList->ip, sizeOfSendBuffer);
            sendErrorCode = WSAGetLastError();
            bool isTimeOut = false;
            char recvBuff[RECV_BUFFER_SIZE];

            while(true){
                if(i++ > 5){    // Try 5 times to receive reply message but timeout
                    isTimeOut = true;
                    break;
                }

                memset(recvBuff, 0, RECV_BUFFER_SIZE);

                recvStatus = recvfrom(sock, recvBuff, MAXBYTE, 0, (SOCKADDR*)&recvSockAddr, &sizeOfRecvSockAddr);

                if (!strcmp(inet_ntoa(recvSockAddr.sin_addr), pDnsList->ip))
                    if ((char)(recvBuff+9) == 0x11)    // IP header + 9 bytes: Protocol, 0x11: UDP
                    break;
            }

            if(isTimeOut)
                throw WinsockRecvTimeOutException();

            char ipInfo = recvBuff[0];
            short* ipMsgLen = (short*)&(recvBuff[2]);
            int ipVer = ipInfo >> 4;
            int ipHeadLen = ((ipInfo << 4) >> 4) * 4;

            char* pUdpHdr = (char*)(recvBuff + ipHeadLen);

            if (*(short*)pUdpHdr == ntohs(53)){    // First 2 bytes of UDP Header is port number
                int udpHdrLen = *(short*)(pUdpHdr + 4);
                dnsMessage* pDnsResp = (dnsMessage*)(pUdpHdr + udpHdrLen);
                dnsAnsMsg* pDnsAnsersMsg = (dnsAnsMsg*)(pUdpHdr + DNS_MESSAGE_SIZE + targetLen + DNS_QUERY_INFO_SIZE);

                // pDnsAnsersMsg->
            }
        }
        closesocket(sock);
    }
    catch (exception& e){
        errOut << e.what();
        return NULL;
    }
    return ip;
} 

/*         char cycle[26];
        for(int i=97; i<=122; i++)
            cycle[i-97] = i;
        char fullFill[size];
        int count = size / 26;
        if (count != 0)
            for(int i=0; i<count; i++)
                memcpy(&fullFill[i*26], &cycle[0], 26);
        memcpy(&fullFill[(count-1) * 26], &cycle[0], size%26); */
/*         for(int i=0; i<size; i++)
            fullFill[i] = rand()+32;    // ASCII:32~126 are displayable
 */

/*             if ((!strcmp(argv[i], "-t") || !strcmp(argv[i], "/t")) && res->count == 0)
                res->loop = true;

            else if (!strcmp(argv[i], "--debug"))
                res->debug = true;

            else if ((!strcmp(argv[i], "-l")) || (!strcmp(argv[i], "/l")) || 
                     (!strcmp(argv[i], "-n")) || (!strcmp(argv[i], "/n"))){
                for (int j=0; j<sizeof(argv[i+1])-1; j++)
                    if (argv[i+1][j] < '0' || argv[i+1][j] > '9')
                        if (argv[i+1][j] != '\0')
                            throw ParaResolveFailedException();

                if (argv[i][1] == 'l')
                    res->size = atoi(argv[++i]);
                else {
                    if(res->loop == false)
                        res->count = atoi(argv[++i]);
                    else
                        throw ParaResolveFailedException();
                }
            } */