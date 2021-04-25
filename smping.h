#include <exception>
#include <vector>
#include <cstdlib>
#include <iostream>

using namespace std;

struct ParaResolveFailedException : public exception{
    const char * what () const throw (){
        return "\nParameters parse error.";
    }
};

struct WinsockRecvTimeOutException : public exception{
    const char * what () const throw (){
        return "\nTimeout......";
    }
};

/* struct WinsockBindFailedException : public exception{
    string result = "\nBind address failed.Error code:";
    WinsockBindFailedException(int errCode){
        result.append(to_string(errCode));
    }
    const char * what () const throw (){
        return result.c_str();
    }
}; */

struct WinDnsApiQueryFailedException : public exception{
    string result = "\nDNS query failed.Error code:";
    WinDnsApiQueryFailedException(int errCode){
        result.append(to_string(errCode));
    }
    const char * what () const throw (){
        return result.c_str();
    }
};

struct WinsockSendException : public exception{
    string result = "\nAn error occurred while send data. Code:";
    WinsockSendException(int errCode){
        result.append(to_string(errCode));
    }
    const char * what () const throw (){
        return result.c_str();
    }
};

struct IcmpRecvFailedException : public exception{
    const char * what () const throw (){
        return "\nIP version check failed.";
    }
};

struct GetNetworkParamsFailedException : public exception{
    int errCode;
    string result = "\nGet network parameters failed.";
    GetNetworkParamsFailedException();
    GetNetworkParamsFailedException(int errCode, int retVal = -1){
        switch (errCode){
            case 1:
                result.append("Allocating memory failed.");
                break;
            case 2:
                result.append("GetNetworkParams() returned a error.Error code:");
                result.append(to_string(retVal));
                break;
        }
    }
    const char * what () const throw (){
        return result.c_str();
    }
};


struct parseResult{
    bool isIPAddr = true;
    bool loop = false;
    bool debug = false;
    string target;
    string ip;
    int count = 0; // Useful in check if -t and -n 
    int size = 32;
};

struct dnsQueryInfo{
    unsigned short type;
    unsigned short dnsClass;
};

struct dnsMessage{
    unsigned short id;
    unsigned short flag;
    unsigned short questions;
    unsigned short ansRR;
    unsigned short authRR;
    unsigned short adlRR;    // Additional RR
};

struct dnsAnsMsg{
    // Skip the head 2 bytes: 1100 0000 0000 1100
    // I don't know why it exists but that's 
    unsigned short head;
    unsigned short type;
    unsigned short dnsClass;
    unsigned int TTL;
    unsigned short len;
    char ip[4];
};

struct icmpHeader{
    unsigned char type; 
    unsigned char code;
    unsigned short CheckSum;
    unsigned short id;
    unsigned short seq;
};

struct pingInfo{
    // char* destIP;
    int lost;
    double minRTT;
    double maxRTT;
    double avgRTT;
};

struct DNSList{
    char* ip;
    DNSList* next = NULL;
};

parseResult* checkArgs(int argc, char* argv[]);
inline const char* bool2Char(bool input);
DNSList* GetDNSList(bool debug, ostream& errOut);
char* NsLookup(string& hostname, bool debug, ostream& errOut);
const char* NsLookupFull(string& hostname, bool debug, ostream& errOut);
unsigned short CheckSum(icmpHeader* head, int len);
// unsigned short chsum(icmpHeader *picmp, int len);
pingInfo* Ping(string& destIP, bool loop, int count, int size, int seqStart, bool debug, ostream& stdOut, ostream& errOut);