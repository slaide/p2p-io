#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<ifaddrs.h>

#define BUFFER_SIZE 1024

void print_local_ip(void){
	struct ifaddrs *ifaddr;
	char ip[INET6_ADDRSTRLEN];

	if(getifaddrs(&ifaddr)==-1){
		perror("getifaddrs");
		return;
	}

	for(auto ifa=ifaddr;ifa!=NULL;ifa=ifa->ifa_next){
		if(ifa->ifa_addr == NULL) continue;

		int family=ifa->ifa_addr->sa_family;
		if(family==AF_INET || family==AF_INET6){
			void*addr;
			if(family==AF_INET){
				addr=&((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
			}else if(family==AF_INET6){
				addr=&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
			}
			
			inet_ntop(family,addr,ip,sizeof(ip));
			printf("interface %s local ip: %s\n",ifa->ifa_name, ip);
		}
	}
	freeifaddrs(ifaddr);
}

void run_server(
	int port
){	
	// start server socket
	int server_fd=socket(AF_INET,SOCK_STREAM,0);
	if(server_fd<0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// configure ip and port to accept requests from (any ip, only configured port)
	struct sockaddr_in server_addr=(struct sockaddr_in){
		.sin_family=AF_INET,
		.sin_addr={
			.s_addr=INADDR_ANY,
		},
		.sin_port=htons(port)
	};
		

	// bind server address to port
	if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		perror("bind failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// init listening for requests
	listen(server_fd,1);
	printf("waiting for connection\n");

	// accept incoming request (and store its info into client_addr).
	// this blocks until a request arrives.
	struct sockaddr_in client_addr;
	socklen_t addr_len=sizeof(client_addr);
	int client_fd=accept(server_fd,(struct sockaddr*)&client_addr,&addr_len);
	if(client_fd<0){
		perror("accept failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	printf("connected. listening\n");

	char buffer[BUFFER_SIZE];
	while(1){
		memset(buffer,0,BUFFER_SIZE);
		// read bytes from client (waits until any bytes arrive, or one of a few events occurs)
		int bytes_received=recv(client_fd,buffer,BUFFER_SIZE,0);
		if(bytes_received<=0){
			printf("client disconnected\n");
			break;
		}
		printf("client: %s\n",buffer);
	}

	close(client_fd);
	close(server_fd);
}

void run_client(
	const char *server_ip,
	int port
){
	// start client
	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0){
		perror("socket creation failed");
		return;
	}

	// create server address
	struct sockaddr_in server_addr={
		.sin_family=AF_INET,
		.sin_port=htons(port),
	};
	inet_pton(AF_INET,server_ip,&server_addr.sin_addr);

	// connect to server
	if(connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		perror("connection failed");
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf("connected. type to send\n");

	char buffer[BUFFER_SIZE];
	while(1){
		// read from stdin and store
		fgets(buffer,BUFFER_SIZE,stdin);

		// stop on control sequence
		if(
			strlen(buffer)==strlen("exit") 
			&& strncmp(buffer,"exit",4)==0
		) break;

		// send input to server (minus trailing newline)
		send(sock,buffer,strlen(buffer)-1,0);
	}

	close(sock);
}

int main(int argc, char**argv){
	// default port
	int port=12344;

	if(argc==2){
		run_client(argv[1],port);
	}else{
		// print available IPs to connect to
		print_local_ip();
		// then start server
		run_server(port);
	}
	
	return 0;
}
