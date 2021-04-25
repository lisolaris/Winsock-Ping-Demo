/* 
 * 使用gcc 9.2.0 (tdm-1) x86, 在Windows 10 x64下编译通过
 * 使用了libws2_32.a静态库，感谢gcc
 * 
 * 使用方法：smping [-t] [-l size] [-n count] [-4] [-6] [--debug] target_name
 * -t 一直ping指定的主机 直至用户强制停止（Ctrl + C）
 * -l size 指定要发送的ICMP报文大小
 * -n count 指定要发送的次数
 * -4 强制使用IPv4
 * -6 强制使用IPv6
 * --debug 输出用于debug的额外信息
 */

/* 
 * 2021/03/31 新建文件 smping.cpp
 * 2021/04/** 摸鱼 摸鱼 摸鱼
 * 2021/04/18 完成基本的ping功能，但不能解析参数
 * 2021/04/19 仔细研究了checkSum() 现在总算算出来是对的了
 * 2021/04/20 重写了checkArgs() 现在可以解析 -t -l -n 参数了；另外还加上了很多（我不喜欢的）大括号，提高可读性
 *            实现了nslookup() 本来想自己发送DNS包查询的，但是卡在第一步 获取DNS服务器上 
 *            大概是我用的gcc链接Windows自带库有问题 又不想把DNS服务器写死在代码里，遂用Winsock提供的api偷懒了
 * 2021/04/21 增加了nslookupFull() 如果nslookup()失败（确实会失败，在解析校内域名时）就自己发送DNS请求查询
 */

//代码中的注释都是英文的 单纯是我懒不想切换输入法

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

// Customed malloc() and free(); See https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getnetworkparams
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
// #define RAND_MAX 126-32    // Number of displayable characters in ASCII

// !!! Must put after #include "smping.h"
// May speed up in a certain degree, I guess :(
const int DnsMsgSize = sizeof(dnsMessage);
const int DnsQueryInfoSize = sizeof(dnsQueryInfo);
const int IcmpHeaderSize = sizeof(icmpHeader);

#define DNS_MESSAGE_SIZE DnsMsgSize
#define DNS_QUERY_INFO_SIZE DnsQueryInfoSize
#define ICMP_HEADER_SIZE IcmpHeaderSize
#define RECV_BUFFER_SIZE 1024
#define LOCAL_PORT 56789

using namespace std;

int main(int argc, char* argv[]){
    // char* testArgv[] = {"smping", "-t", "223.5.5.5"};
    // int testArgc = 3;
    // char* testArgv[] = {"-t", "--debug", "223.5.5.5"};
    // int testArgc = 3;
    try{
        parseResult* parseRes = checkArgs(argc, argv);
        pingInfo* pingRes = NULL;

        if (parseRes->debug){
            cerr << "argc: " << argc << endl;
            for(int i=0; i<argc; i++)
                cerr << "argv[" << i << "] " << argv[i] << endl;
        
        if (parseRes->loop)
            parseRes->count = numeric_limits<int>::max();

            cerr << endl;
            cerr << "Parameters resolve result:" << endl
                 << "target.isIPAddr = " << bool2Char(parseRes->isIPAddr) << endl
                 << "Loop = " << bool2Char(parseRes->loop) << endl
                 << "Debug enabled = " << bool2Char(parseRes->debug) << endl
                 << "Target in parameter = " << parseRes->target << endl
                 << "IP address of target = " << parseRes->ip << endl
                 << "Test count = " << parseRes->count << endl
                 << "ICMP message size = " << parseRes->size << endl
                 << endl;
        }

        if (!(parseRes->isIPAddr))
            parseRes->ip = string(nslookup(parseRes->target, parseRes->debug, cerr));
        else
            parseRes->ip = parseRes->target;

        cout << "PING " << parseRes->target << " (" << parseRes->ip << ") "
             << parseRes->size << '(' << (parseRes->size)+28 << ')'
             << " bytes of data:"
             << endl;

        ping(parseRes->ip, parseRes->loop, parseRes->count, parseRes->size, 1, parseRes->debug, cout, cerr);
/*         for (int i=0; i<parseRes->count; i++){
            pingRes = ping(parseRes->ip, parseRes->loop, parseRes->size, i+1, parseRes->debug, cerr);
            cout << pingRes->len << " bytes from " << parseRes->ip << ':'
                 << " ICMP_sequence=" << pingRes->seq
                 << " TTL=" << pingRes->ttl
                 << " Time=" <<fixed << setprecision(3) << pingRes->durTime << " ms";
            if (parseRes->debug)
                cerr << " CheckSum=" << pingRes->checksum << endl;
            else
                cout << endl;

            Sleep(500);
        } */

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
parseResult* checkArgs(int argc, char* argv[]){
    parseResult* res = new parseResult;

    if (argc == 1)     // If not give parameter 
        throw ParaResolveFailedException();
    else {
        res->target.assign(argv[argc-1]);

        for (int i=1; i <= (argc-2); i++){
            // cout << "checkArgs(): " << endl;
            // cout << "argv[" << i << "] " << argv[i] <<endl;
            char* current = argv[i];
            // Accpet - / \ to specified parameters
            if (current[0] == '-' || current[0] == '/' || current[0] == '\\')
                if (current[1] == 't')
                    res->loop = true;

                else if ((string(current)).find("debug") != string::npos)
                    res->debug = true;

                else if (current[1] == 'l' || current[1] == 'n'){
                    for (int j=0; j<sizeof(argv[i+1])-1; j++)
                        if (argv[i+1][j] < '0' || argv[i+1][j] > '9')
                            if (argv[i+1][j] != '\0')
                                throw ParaResolveFailedException();

                    if (argv[i][1] == 'l')
                    res->size = atoi(argv[++i]);
                    else
                        if(res->loop == false)
                            res->count = atoi(argv[++i]);
                        else
                            throw ParaResolveFailedException();
                }

        }
    }
    // Count the '.' appeared in target, to check whether IP address or hostname
    const char* temp = (res->target).c_str();
    int dotCount = 0;
    for (int i=0; i<(res->target).size(); i++){
        if (temp[i] < '0' || temp[i] > '9'){
            if (temp[i] == '.')
                dotCount++;
            else
                res->isIPAddr = false;
        }
    }
    if (dotCount != 3 && res->isIPAddr == true) throw ParaResolveFailedException();
    if (res->count == 0 && res->loop == false) res->count = 4;
    return res;
}

// resolve the input hostname to IP address, use gethostbyname() in Winsock library
char* nslookup(string& hostname, bool debug = false, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "nslookup() called" << endl;

    // If use string to declare ip then return it's reference, the Program will exit unexpectly
    char* ip = NULL;
    try{
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        hostent* host = gethostbyname(hostname.c_str());

        if (host == NULL){
            if (debug){
                errOut << "Winsock gethostbyname() failed.";
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
                errOut << "Try nslookupFull() to get target IP." << endl;
            }
            ip = nslookupFull(hostname, debug, errOut);
        }
        else
            ip = inet_ntoa(*(in_addr*)host->h_addr);

        if (debug)
            errOut << endl << "IP resolved: " << ip << endl << endl;
    }
    catch (exception& e){
    }
    return ip;
}

char* nslookupFull(string& hostname, bool debug = false, ostream& errOut = cerr){
    char* ip;
}

// Get current DNS configurations, using GetNetworkParams()
/* DNSList* getDNSList(bool debug = false, ostream& errOut = cerr){
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
} */

// Send DNS query message to get target IP, will be called if nslookup() failed
/* char* nslookupFull(string& hostname, bool debug = false, ostream& errOut = cerr){
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

                pDnsAnsersMsg->
            }
        }
        closesocket(sock);
    }
    catch (exception& e){
        errOut << e.what();
        return NULL;
    }
    return ip;
} */

// Ping destnation IP, default to 32 Byte data pack and repeat for 4 times
pingInfo* ping(string& destIP, bool loop = false, int count = 4, int size = 32, int seqStart = 1, bool debug = false, ostream& stdOut = cout, ostream& errOut = cerr){
    if (debug)
        errOut << endl << "ping() called" << endl;

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

        icmpHeader* pIcmpReqHdr = (icmpHeader*)sendBuff;    // set icmp Head
        pIcmpReqHdr->type = 0x08;    // ICMP Type: request echo
        pIcmpReqHdr->code = 0;
        pIcmpReqHdr->id = (USHORT)::GetCurrentProcessId();    //ICMP id: usually used to identify which process send icmp request
        // pIcmpReqHdr->seq = seqStart;
        pIcmpReqHdr->checkSum = 0;

        // Fullfill the rest part in icmp package
        memset((char*)(sendBuff+ICMP_HEADER_SIZE), 'a', size);

        sockaddr_in recv;
        int recvLen = sizeof(recv);
        char recvBuff[RECV_BUFFER_SIZE];
        int sendStatus, recvStatus, sendErrorCode;
        bool isTimeOut = false;

        long startClock, endClock;

        // Cycled pinging target
        for(int times=0; times<count; times++){
            pIcmpReqHdr->seq = seqStart + times;
            pIcmpReqHdr->checkSum = checkSum(pIcmpReqHdr, sizeof(sendBuff));

            startClock = clock();

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

                int recvStatus = recvfrom(sock, recvBuff, MAXBYTE, 0, (SOCKADDR*)&recv, &recvLen);

                if(strcmp(inet_ntoa(recv.sin_addr), destIP.c_str()) == 0){
                    resps++;
                    break;
                }
            }
            endClock = clock();

            // Set debug log after timer to avoid extra delay
            if (debug)
                errOut << '\t' << "Send info: " << "status: " << sendStatus 
                       << " Error code: " << sendErrorCode
                       << endl
                       << '\t' << "Receive info: " << "status: " << recvStatus 
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
            icmpHeader* pIcmpResp = (icmpHeader*)(recvBuff + ipHeadLen);

            // pingInfo* result = new pingInfo;

            if(pIcmpResp->type == 0){    // ICMP echo reply message
                double durTime = 1000*(double)((endClock-startClock)/double(CLOCKS_PER_SEC));
                // result->seq = seqStart;
                // result->len = ntohs(*ipMsgLen);
                // result->ttl = recvTTL;
                int checksum = ntohs(pIcmpResp->checkSum);

                stdOut << ntohs(*ipMsgLen) << " bytes from " << destIP << ':'
                    << " ICMP_sequence=" << pIcmpReqHdr->seq
                    << " TTL=" << int(recvTTL)
                    << " Time=" <<fixed << setprecision(3) << durTime << " ms";

                if (debug)
                    errOut << " CheckSum=" << checksum << endl;
                else
                    stdOut << endl;
            }else{
                throw IcmpRecvFailedException();
            }

        Sleep(500);
        }

        closesocket(sock);
        // return result;

    }catch(WinsockRecvTimeOutException& e){
        errOut << e.what();
    }catch(IcmpRecvFailedException& e){
        errOut << e.what();
    }
    return NULL;
}

// Calculate the checksum from ICMP header data
unsigned short checkSum(icmpHeader* head, int len){
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