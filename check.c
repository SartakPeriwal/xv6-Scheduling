#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
        int fork_id = fork();
        if (fork_id == 0) {
                    double d = 1, x = 0;
                    for (int i = 0; i < 300; ++i) {
                        for (int j = 0; j < 50; ++j) {
                            for (int k = 0; k < 200000; k += d) {
                                                    for(int l=0;l<2;l++)
                                                                              x = x + 102;
                                                                    
                            }
                                        
                        }
                                
                    }
                            printf(1, "Test function ended\n");
                                
        }
            exit();
            
}
