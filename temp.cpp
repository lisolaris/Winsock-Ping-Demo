//这个文件用来存一些我不舍得扔掉的代码

#include "smping.h"
#include <iostream>

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


/* unsigned short chsum(icmpHeader *picmp, int len) {
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
} */

/* // checksum() from Microsoft offical example
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
 */
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