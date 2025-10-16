#include <stdio.h>
#include <syslog.h>

int main(int argc, char **argv){
    char* writefile;
    char* writestr;
    FILE* fwritefile;


    openlog(NULL, 0, LOG_USER);

    if(argc < 2){
        syslog(LOG_ERR, "Usage: %s <filename> <write_string>",argv[0]);
        return 1;
    }

    writefile = argv[1];
    writestr = argv[2];

    fwritefile = fopen(writefile, "w+");
    if(fwritefile == NULL){
        syslog(LOG_ERR, "Error opening file %s", writefile);
        return 1;
    }
    fprintf(fwritefile, "%s\n", writestr);
    syslog(LOG_DEBUG,"Writing %s to %s", writestr, writefile);

    closelog();
    fclose(fwritefile);
    return 0;
}