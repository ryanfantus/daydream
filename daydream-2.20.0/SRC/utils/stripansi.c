#include <stdio.h>

static void stripansi(char *);

int main(int argc, char *argv[])
{
	char foo[4096];
	
	while (fgets(foo,4096,stdin)) {
		stripansi(foo);
		fputs(foo,stdout);
	}
	return 0;
}

static void stripansi(char *tempmem)
{
	int go=1;
	int s=0;
	int t=0;
	int u;
	
  	while (go) {
   		if (tempmem[s]==27) {
    			u=s;
  			s=s+2;
skiploop:
			if (s) goto go1;
			go=0;
			continue;
go1:
			if (tempmem[s] >= 'A') goto go2;
			s++;
			goto skiploop;
go2:
			if (tempmem[s] == 'm') goto go3;
			s=u; tempmem[t]=tempmem[s]; t++;
go3:
			s++;
		} else {
			tempmem[t]=tempmem[s]; t++;s++; 
			if(tempmem[s]==0) go=0; 
    		}
	}
	tempmem[t]=0;
}
