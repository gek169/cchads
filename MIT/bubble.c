#include<stdio.h>
#include<string.h>
i;d;
main(int c,char** a){
for(d++,c--;d;)for(i=0,d=0;++i<c;)if(strcmp(a[i],a[i+1])<0){*a=a[i];a[i]=a[i+1];a[i+1]=*a;d=1;}
for(i=0;++i<c+1;)puts(a[i]);
}
