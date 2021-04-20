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

struct icmpRecvFailedException : public exception{
    const char * what () const throw (){
        return "IP version check failed.";
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

parseResult* checkArgs(int argc, char* argv[]);
inline const char* bool2Char(bool input);
char* nslookup(string& hostname, bool debug, ostream& errOut);
unsigned short checkSum(icmpHeader* head, int len);
// unsigned short chsum(icmpHeader *picmp, int len);
pingInfo* ping(string& destIP, bool loop, int size, int seq, bool debug, ostream& errOut);