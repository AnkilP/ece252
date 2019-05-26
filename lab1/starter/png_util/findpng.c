/**
 * @file: findpng.c
 * @brief: recursively finds png files in a given directory 
  */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */

/**
 *GLOBAL VARIABLES
 */

//char * paths[];
void lastchar_cut(char *str)
{
    if(str[strlen(str) - 1] == '/'){
        str[strlen(str)-1] = 0;
    }
}

#define CHUNK 8 /* read 8 bytes at a time */
//char png_magic_number[3] = {"0x50", "0x4e", "0x47"};


int isPNG(char * path){
    char buf[CHUNK];
    int x = 0;
    FILE* f = fopen( path, "r");
    if ( f == NULL ) {
            //printf("Unable to open file! %s is invalid name?\n", argv[1] );
            return 0;
    }   
    
    fread(buf, 1, sizeof(buf), f);
    if(buf[1] == 0x50 && buf[2] == 0x4e && buf[3] == 0x47){
           x = 1;
    } 
    fclose( f );
    return x;
}

void query_files(char * path){
  DIR *p_dir;
  struct dirent *p_dirent;
  char str[64];
  int found = 0;
    if (path == NULL) {
        fprintf(stderr, "Usage: %s <directory name>\n", path);
        exit(1);
    }
    
    if ((p_dir = opendir(path)) == NULL) {
        sprintf(str, "opendir(%s)", path);
        perror(str);
        exit(2);
    }
    
    lastchar_cut(path); // this shouldnt really be const, now should it?

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */
        if(strcmp(str_path, ".") != 0 && strcmp(str_path, "..") != 0){
             //printf("name %i", p_dirent->d_type);   
             if(p_dirent->d_type == 4){
//                printf("name %s", path);
                memset(str, 0, 64);
                strcat(str, path);
                strcat(str,"/");
                strcat(str, str_path);
                query_files(str);
                memset(str, 0, 64);
             }
            if (str_path == NULL) {
                fprintf(stderr,"Null pointer found!"); 
                exit(3);
            } else {
                found++;
                if(p_dirent->d_type != 4){   
                    memset(str, 0, 64);
                    strcat(str, path);
                    strcat(str, "/");
                    strcat(str, str_path); 
                    if(isPNG(str) == 1){
                        printf("%s\n", str);
                    }
                    memset(str, 0, 64);
                    //fflush(stdout);
                }
            }
        }
    }

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    } 

    if(found == 0){
        printf("No PNG file found");
    }

}

int main(int argc, char *argv[]) 
{
//    char str[64];
    if(argc != 1){  
       query_files(argv[1]);
    }
    else{
       int cue = isPNG("../images/WEEF_1.png");
       printf("%i", cue); 
    } // testing purposes
    return 0;
}
