#include <stdlib.h>                                                                                              
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char ** argv){
    int c;
    int t = 1;
    int n = 3;
    char * str = "option requires an argument";

    while((c = getopt(argc, argv, "t:n:")) != -1) {
        switch(c) {
        case 't':
        t = strtoul(optarg, NULL, 10);
        printf("option -t specifies a value of %d. \n", t);
        if(t <= 0){
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        
        case 'n':
            n = strtoul(optarg, NULL, 10);
            printf("option -n specifies a value of %d. \n", n);
            if(n <= 0 || n > 3){
                fprintf(stderr, "not enough images");
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
}

