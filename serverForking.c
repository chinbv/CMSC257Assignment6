#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define MY_PORT_ID 3000
#define MAXLINE 50
#define MAXSIZE 50

#define ACK                   2
#define NACK                  3
#define REQUESTFILE           100
#define COMMANDNOTSUPPORTED   150
#define COMMANDSUPPORTED      160
#define BADFILENAME           200
#define FILENAMEOK            400

char *terminationString;
int socketid;

int written(int sd,char *ptr,int size);
int readn(int sd,char *ptr,int size);

void signal_handler (int signum) {
  printf(" Sig handler got a [%d]\n", signum);
  printf("Exiting...\n");

  close(socketid);

  exit(0);
}

int main(int argc, char **argv)  {

  signal(SIGABRT, signal_handler);//int = 6
  signal(SIGQUIT, signal_handler);//int = 3
  signal(SIGTERM, signal_handler);//int = 15
  signal(SIGKILL, signal_handler);//int = 9
  signal(SIGINT, signal_handler);//int = 2

  int newsd, pid, clientLength;
  struct sockaddr_in my_addr, client_addr;

  terminationString = "cmsc257";

  printf("server: creating socket\n");
  if ((socketid = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    printf("server: socket error : %d\n", errno); exit(0);
  }

  printf("server: binding my local socket\n");
  bzero((char *) &my_addr,sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(MY_PORT_ID);
  my_addr.sin_addr.s_addr = htons(INADDR_ANY);

  if (bind(socketid ,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0) {
    printf("server: bind  error :%d\n", errno); exit(0);
  }
  printf("server: starting listen \n");

  if (listen(socketid,5) < 0) {
    printf("server: listen error :%d\n",errno);exit(0);
  }

  while(1) {
    /* Accept connection, create child */
    /* loop and wait for another connection                  */
    printf("server: starting accept\n");
    if ((newsd = accept(socketid ,(struct sockaddr *) &client_addr,
    &clientLength)) < 0) {
      printf("server: accept  error :%d\n", errno);
      exit(0);
    }
    printf("server: return from accept, socket for this ftp: %d\n",
    newsd);
    if ( (pid=fork()) == 0) {
      /* Child starts here. FILE TRANSFER */
      close(socketid);   /* child shouldn't do an accept */
      doftp(newsd);
      close(newsd);
      printf("child exiting...\n");
      exit(0);         /* work is completed by child */
    }
    close(newsd);
    /* Parent contines */
    printf("parent continues\n================\n\n");
  }

}//End of Main


/* CHILD, FILE TRANSFER */
doftp(int newsd) {
  int i,fsize,fd,msg_ok,filenameResponse,req,c,ack;
  int number_read ,num_blks , num_blks1,num_last_blk,num_last_blk1,tmp;
  char fname[MAXLINE];
  char out_buf[MAXSIZE];
  FILE *transferFile;

  number_read = 0;
  num_blks = 0;
  num_last_blk = 0;


  /* START SERVICING THE CLIENT */

  /* get command code from client.*/
  req = 0;
  if((readn(newsd,(char *)&req,sizeof(req))) < 0) {
    printf("server: read error %d\n",errno);
    exit(0);
  }
  req = ntohs(req);
  printf("server: client request code is: %d\n",req);
  if (req!=REQUESTFILE) {
    printf("server: unsupported operation. \n");
    msg_ok = COMMANDNOTSUPPORTED;
    msg_ok = htons(msg_ok);
    if((written(newsd,(char *)&msg_ok,sizeof(msg_ok))) < 0) {
      printf("server: write error :%d\n",errno);
      exit(0);
    }
    exit(0);
  }

  /* reply to client: command OK */
  msg_ok = COMMANDSUPPORTED;
  msg_ok = htons(msg_ok);
  if((written(newsd,(char *)&msg_ok,sizeof(msg_ok))) < 0) {
    printf("server: write error :%d\n",errno);
    exit(0);
  }

  filenameResponse = FILENAMEOK;
  if((read(newsd,fname,MAXLINE)) < 0) {

    printf("server: filename read error :%d\n",errno);
    filenameResponse = BADFILENAME ;

  }

  /* File can't open, notify client, terminate */
  if((transferFile = fopen(fname,"r")) == NULL) {
    /*cant open file*/
    printf("could not open file %s, error code %d\n", fname, errno);
    filenameResponse = BADFILENAME;
  }

  tmp = htons(filenameResponse);
  if((written(newsd,(char *)&tmp,sizeof(tmp))) < 0) {
    printf("server: write error :%d\n",errno);
    exit(0);
  }

  if(filenameResponse == BADFILENAME) {
    printf("server cant open file\n");
    close(newsd);
    exit(0);
  }

  printf("server: filename is %s\n",fname);

  req = 0;
  if ((readn(newsd,(char *)&req,sizeof(req))) < 0) {
    printf("server: read error :%d\n",errno);
    exit(0);
  }
  printf("server: start transfer command, %d, received\n", ntohs(req));


  /*Server gets the filesize and calculates the number of blocks it will take to
  transfer the file.
  Calculates number of bytes in the last partially filled block if there are any.
  This information is sent to the client. */

  printf("server: starting transfer\n");
  fsize = 0; ack = 0;

  while ((c = getc(transferFile)) != EOF) {
    fsize++;
  }

  num_blks = fsize / MAXSIZE;
  num_blks1 = htons(num_blks);
  num_last_blk = fsize % MAXSIZE;
  num_last_blk1 = htons(num_last_blk);

  rewind(transferFile);

  /* File begins to be transfered (Block by Block)*/

// printf("num_blks %d = fsize %d / MAXSIZE %d\n", num_blks, fsize, MAXSIZE);

  for(i= 0; i < num_blks; i ++) {
    // printf("Reading block number %d\n", i);

    number_read = fread(out_buf,sizeof(char), MAXSIZE, transferFile);
    // printf("number_read %d\n", number_read);
    if (number_read == 0) {
      printf("server: file read error\n");
      exit(0);
    }

    if (number_read != MAXSIZE) {
      printf("server: file read error : number_read is less\n");
      exit(0);
    }

    //printf(".");


        // printf("out buffer:\n");

        // int i;
        // for( i=0; i < number_read; i++) {
        //   printf(":%c", out_buf[i]);
        // }
        // printf("\nend of buffer\n");


    if((written(newsd,out_buf,MAXSIZE)) < 0) {//writes to client
      printf("server: error sending block:%d\n",errno);
      exit(0);
    }

    // printf(" %d...", i);

  }


  if (num_last_blk > 0) {
    number_read = fread(out_buf,sizeof(char),num_last_blk,transferFile);

    if (number_read == 0) {
      printf("server: file read error\n");
      exit(0);
    }

    if (number_read != num_last_blk) {
      printf("server: file read error : number_read is less 2\n");
      exit(0);
    }

    // printf("out buffer:\n");
    //
    // int i;
    // for( i=0; i < number_read; i++) {
    //   printf(":%c", out_buf[i]);
    // }
    // printf("\nend of buffer\n");


    if((written(newsd,out_buf,num_last_blk)) < 0) {//writes to client
      printf("server: file transfer error %d\n",errno);
      exit(0);
    }

    //Termination string to be passed to client to terminate
    printf("server: Writing completed. Sending terminationString: %s\n", terminationString);
    if((written(newsd,terminationString, strlen(terminationString))) < 0) {//writes to client
      printf("server: sending termination string error %d\n",errno);
      exit(0);
    }

    if((readn(newsd,(char *)&ack,sizeof(ack))) < 0) {
      printf("server: Acknowledgment read  error %d\n",errno);
      exit(0);
    }

    if (ntohs(ack) != ACK) {
      printf("server: Acknowledgment not received last block\n");
      exit(0);
    } else {
      printf("received ack\n");
    }
  } else {
    printf("\n");
  }

  /* end of file transfer */
  printf("server: The file transfer was completed on socket %d\n",newsd);
  fclose(transferFile);
  close(newsd);
}//end of doftp

int readn(int sd,char *ptr,int size) {
  int number_left, number_read;
  number_left = size;

  printf("readn of size %d\n", size);

  while (number_left > 0) {
    number_read = read(sd, ptr, number_left);

    if(number_read < 0) {
      return(number_read);
    }

    if (number_read == 0) {
      break;
    }
    number_left -= number_read;
    ptr += number_read;
  }

  return(size - number_left);
}

int written(int sd,char *ptr,int size) {
  int number_left,no_written;
  number_left = size;
  while (number_left > 0) {
    no_written = write(sd,ptr,number_left);
    if(no_written <=0) {

      return(no_written);

    }
    number_left -= no_written;
    ptr += no_written;
  }

  return(size - number_left);

}
