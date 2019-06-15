#include <stdlib.h>                                                                                              
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "catpng.h" //for concatenatePNG()

int main(int argc, char ** argv){
    int c;
    int num_threads = 1;
    int img_id = 1;
    char * str = "option requires an argument";

    while((c = getopt(argc, argv, "t:n:")) != -1) {
        switch(c) {
        case 't':
        num_threads = strtoul(optarg, NULL, 10);
        printf("option -t specifies a value of %d. \n", num_threads);
        if(t <= 0){
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        
        case 'n':
            img_id = strtoul(optarg, NULL, 10);
            printf("option -n specifies a value of %d. \n", img_id);
            if(n <= 0 || n > 3){
                fprintf(stderr, "image choice is invald");
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
}

