#include <exception>
#include <vector>
#include <iostream>

using namespace std;

struct ParaResolveFailedException : public exception{
    const char * what () const throw (){
        return "Parameters parse error.";
    }
};

struct WinsockRecvTimeOutException : public exception{
    const char * what () const throw (){
        return "Timeout......";
    }
};

struct WinsockSendException : public exception{
    int errCode;
    WinsockSendException(int errCode){
        errCode = errCode;
    };
    const char * what () const throw (){
        return "An error occurred while send data. Code:";
    }
};

struct IcmpRecvFailedException : public exception{
    const char * what () const throw (){
        return "IP version check failed.";
    }
};

struct GetNetworkParamsFailedException : public exception{
    int errCode;
    string result = string("Get network parameters failed.");
    GetNetworkParamsFailedException();
    GetNetworkParamsFailedException(int errCode, int retVal = -1){
        switch (errCode){
            case 1:
                result.append("Allocating memory failed.");
                break;
            case 2:
                result.append("GetNetworkParams() returned a error.Error code:");
                result.append(1, char(retVal - 48));
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

struct icmpHeader{
    unsigned char type; 
    unsigned char code;
    unsigned short checkSum;
    unsigned short id;
    unsigned short seq;
};

struct pingInfo{
    // char* destIP;
    double durTime;
    int seq;
    int len;
    int ttl;
    unsigned short checksum;
};

struct DNSList{
    char* ip;
    DNSList* next = NULL;
};

parseResult* checkArgs(int argc, char* argv[]);
inline const char* bool2Char(bool input);
DNSList* getDNSList(bool debug, ostream& errOut);
char* nslookup(string& hostname, bool debug, ostream& errOut);
char* nslookupFull(string& hostname, bool debug, ostream& errOut);
unsigned short checkSum(icmpHeader* head, int len);
// unsigned short chsum(icmpHeader *picmp, int len);
pingInfo* ping(string& destIP, bool loop, int size, int seq, bool debug, ostream& errOut);