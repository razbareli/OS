#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256
#define MAXHOSTNAME 256
#define MAXCLIENTS 5


void error_handler(std::string const& msg){
  std::cerr<<"system error: " << msg << std::endl;
  exit(EXIT_FAILURE);
}

int establish(unsigned short portnum) {
  char myname[MAXHOSTNAME+1];
  int s;
  struct sockaddr_in sa{};
  struct hostent *hp;
  //hostent initialization
  gethostname(myname, MAXHOSTNAME);
  hp = gethostbyname(myname);
  if (hp == nullptr){
    return -1;
  }
  //sockaddrr_in initlization
  memset(&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = hp->h_addrtype;
  /* this is our host address */
  memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  /* this is our port number */
  sa.sin_port= htons(portnum);
  /* create socket */
  if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0){
    error_handler ("error creating socket in establish");
  }

  if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
    close(s);
    error_handler ("error with bind function in establish");
  }

  listen(s, 3); /* max # of queued connects */
  return(s);
}

int get_connection(int s) {
  int t; /* socket of connection */

  if ((t = accept(s, nullptr, nullptr)) < 0)
    error_handler ("error with accept in get_connection");
  return t;
}

int call_socket(char *hostname, unsigned short portnum) {

  struct sockaddr_in sa{};
  struct hostent *hp;
  int s;

  if ((hp= gethostbyname (hostname)) == NULL) {
      error_handler ("error with gethostbyname in call socket");
  }

  memset(&sa,0,sizeof(sa));
  memcpy((char *)&sa.sin_addr , hp->h_addr ,
         hp->h_length);
  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons((u_short)portnum);
  if ((s = socket(hp->h_addrtype,
                  SOCK_STREAM,0)) < 0) {
    error_handler ("error creating socket in call socket");
  }

  if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
    close(s);
    error_handler ("error with connect in call_socket");
  }
  return(s);
}

long read_data(int s, char *buf, int n) {
  long bcount;       /* counts bytes read */
  long br;               /* bytes read this pass */
  bcount= 0; br= 0;

  while (bcount < n) { /* loop until full buffer */
    br = read(s, buf, n-bcount);
    if (br > 0)  {
      bcount += br;
      buf += br;
    }
    if (br < 0) {
      return -1;
    }
    if (br==0){
      break;
    }
  }
  return(bcount);
}

int client(int port, char* cmd){
  char name[MAXHOSTNAME + 1];
  gethostname (name, MAXHOSTNAME);
  int client_socket = call_socket (name, port);
  if (client_socket < 0){
    return -1;
  }
  write(client_socket, cmd, strlen(cmd));
  close(client_socket);
  return 0;
}

void server (int port){
  char buff[BUFFER_SIZE];
  memset (&buff, 0, BUFFER_SIZE);

  int server_socket = establish (port);
  if (server_socket < 0){
    return;
  }

  fd_set clientsfds;
  fd_set readfds;

  FD_ZERO(&clientsfds);
  FD_SET(server_socket, &clientsfds);

  while(true){
    readfds = clientsfds;
      for (int i = server_socket + 1; i < server_socket + MAXCLIENTS; ++i){
          if (i == server_socket){
              continue;
          }
          if (FD_ISSET(i, &readfds)){
            int r = read_data (i, buff, BUFFER_SIZE);
              if (r < 0){
                return;
              }
              system (buff);
              close(i);

              memset (&buff, 0, BUFFER_SIZE);
              FD_CLR(i, &readfds);
          }
        }
        if (FD_ISSET(server_socket, &readfds)) {
          //will also add the client to the clientsfds
          int client_socket = get_connection (server_socket);
          FD_SET(client_socket, &clientsfds);
        }
  }
}

int main(int argc, char* argv[]){
  int port = std::stoi (argv[2]);

  if (std::string(argv[1]) == "client"){
    client(port, argv[3]);
  }

  if (std::string(argv[1]) == "server"){
      server(port);
  }
  return 0;

}
