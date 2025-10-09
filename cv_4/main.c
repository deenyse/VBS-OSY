#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <wait.h>  // Include for waitpid
#include <time.h>

#define NUM_CHILDREN 5
#define BUFFER_SIZE 6

int main(void)
{
    printf("\n");

    srand48(time(NULL));

    int pipes[NUM_CHILDREN][2];
    pid_t pids[NUM_CHILDREN];

    for (int i = 0; i < NUM_CHILDREN -1; i++)//generate pipes
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe error");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        // printf("[rodič] Spouštím potomka %d\n", i + 1);

        pids[i] = fork();
        if (pids[i] == -1)
        {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0)
        {
            if (i == 0)
            {                       // child 1
                close(pipes[0][0]);
                // close(pipes[0][1]);

                close(pipes[1][0]);
                close(pipes[1][1]);
                
                close(pipes[2][0]);
                close(pipes[2][1]);
                
                close(pipes[3][0]);
                close(pipes[3][1]);
                
                 
                for (int i =0; i < 20; i++){
                    double num = drand48()*100;
                    
                    printf("[gen] Posílám: %.3f\n", num);

                    char out[BUFFER_SIZE];
                    sprintf(out, "%.3f", num);



                    write(pipes[0][1], out, BUFFER_SIZE);
                    usleep(200 * 1000);
                }
                write(pipes[0][1], "END", 4);
                close(pipes[0][1]);
                exit(EXIT_SUCCESS);
            }
            else if (i == 1) // child 2
            {
                // close(pipes[0][0]);
                close(pipes[0][1]);

                close(pipes[1][0]);
                // close(pipes[1][1]);
                
                close(pipes[2][0]);
                close(pipes[2][1]);
                
                close(pipes[3][0]);
                close(pipes[3][1]);
                
                 
                char buf[BUFFER_SIZE];

                while (read(pipes[0][0], buf, BUFFER_SIZE) > 0)
                {
                    if (!strcmp(buf, "END"))
                        break;

                    double num = atof(buf);
                    if(num > 50){
                        printf("[filtr] Přijal: %.3f → zahazuji\n\n", num);
                        continue;
                    }

                    printf("[filtr] Přijal: %.3f → posílám dál\n", num);
                    
                    char out[BUFFER_SIZE];
                    sprintf(out, "%.3f", num);

                    write(pipes[1][1], out, BUFFER_SIZE);
                }
                write(pipes[1][1], "END", 4);

                close(pipes[1][1]);
                close(pipes[0][0]);

                exit(EXIT_SUCCESS);
            }
            else if (i == 2) // child 3
            {
                close(pipes[0][0]);
                close(pipes[0][1]);

                // close(pipes[1][0]);
                close(pipes[1][1]);
                
                close(pipes[2][0]);
                // close(pipes[2][1]);
                
                close(pipes[3][0]);
                close(pipes[3][1]);
                
                 
                char buf[BUFFER_SIZE];

                while (read(pipes[1][0], buf, BUFFER_SIZE) > 0)
                {
                    if (!strcmp(buf, "END"))
                        break;
                        

                    double num = atof(buf);
                    double outNum = sqrt(num) * 10;


                    char out[BUFFER_SIZE];

                    sprintf(out, "%.3f", outNum);

                    printf("[transf] Přijal: %.3f → posílám: %.3f\n", num, outNum);
                    write(pipes[2][1], out, BUFFER_SIZE);
                }
                write(pipes[2][1], "END", 4);

                close(pipes[1][0]);
                close(pipes[2][1]);
                exit(EXIT_SUCCESS);
            }
            else if (i == 3) // child 4
            {
                close(pipes[0][0]);
                close(pipes[0][1]);

                close(pipes[1][0]);
                close(pipes[1][1]);
                
                // close(pipes[2][0]);
                close(pipes[2][1]);
                
                close(pipes[3][0]);
                // close(pipes[3][1]);

 

                char buf[BUFFER_SIZE];

                int count = 0;
                double sum = 0;

                while (read(pipes[2][0], buf, BUFFER_SIZE) > 0)
                {
                    if (!strcmp(buf, "END"))
                        break;
                        
                    double num = atof(buf);
                    count++;
                    sum += num;


                    char out[BUFFER_SIZE *2 +1];
                    sprintf(out, "%.3f %.3f", sum, sum/count);

                    printf("[acc] Přijal: %.3f → součet=%.3f, průměr=%.3f → posílám: \"%.3f %.3f\"\n", num, sum, sum/count, sum, sum/count);
                    
                    write(pipes[3][1], out, BUFFER_SIZE *2 + 1);
                }
                write(pipes[3][1], "END", 4);
                close(pipes[2][0]);
                close(pipes[3][1]);
                exit(EXIT_SUCCESS);
            }
            else if (i == 4) // child 5
            {
                close(pipes[0][0]);
                close(pipes[0][1]);

                close(pipes[1][0]);
                close(pipes[1][1]);
                
                close(pipes[2][0]);
                close(pipes[2][1]);
                
                // close(pipes[3][0]);
                close(pipes[3][1]);

 
                
                char buf[BUFFER_SIZE * 2 + 1];
                FILE* fileStream = fopen("stats.txt", "w+");
                while (read(pipes[3][0], buf, BUFFER_SIZE * 2 + 1) > 0)
                {
                    if (!strcmp(buf, "END"))
                        break;
                        

                    char *space_pos = strchr(buf, ' ');
                    if (space_pos != NULL) {
                        *space_pos = '\0'; 
                    }
                    double sum = atof(buf);
                    double avg = atof(space_pos + 1);  


                    
                    fprintf(fileStream, "[output] %.3f (avg=%.3f)\n",sum, avg);                    
                    printf("[output] %.3f (avg=%.3f)\n\n",sum, avg);

                }

                close(pipes[3][0]);
                exit(EXIT_SUCCESS);
            }
        }
    }

    for (int i = 0; i < NUM_CHILDREN - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        waitpid(pids[i], NULL, 0);  
    }

    return 0;
}