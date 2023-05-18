#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	
    char vector[50];
    //set_color test
	/*set_color(5,6);*/
     
    
    
    //shmat test
    write(1,"\nShmat test:\n",12);
    //free page case
    write(1,"\nfree page case, page = ",24);
    int page = shmat(0,(void*)0x00130000);
    itoa(page,buff);
    write(1,buff,strlen(buff));
    //occupied page case
    write(1,"occupied? page case, page = ",20);
    page = shmat(0,(void*)0x00060000);
    itoa(page,buff);
    write(1,buff,strlen(buff));
    int pid_hijo = fork();
    if (pid_hijo == 0) {
        //son case
        write(1,"\nson case, page = ",17);
        page = shmat(0,(void*)0x00060000);
        itoa(page,buff);
        write(1,buff,strlen(buff));
        exit();
    }
    //page in fork range case
    write(1,"\npage in fork range case, page = ",32);
    page = shmat(0,(void*)0x0011C000);
    itoa(page,buff);
    write(1,buff,strlen(buff));
    //shmdt and shmrm
    page = shmat(1,(void*)0x00135000);
    shmrm(1);
    shmdt((void*)0x00135000);


  while(1) {
  	int res = read(&vector[0],1);
  	itoa(res,buff);
	//write(1,vector,strlen(vector));
	if (res > 0) write(1,vector,res);
   }
}
