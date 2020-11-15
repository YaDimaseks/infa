#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>
int main(){
	char wd[1000];
    	getcwd(wd, sizeof(wd));
	printf("%s\n", wd);
}
