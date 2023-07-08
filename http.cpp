#include<stdio.h>
#include<string.h>
#include<WinSock2.h>
#include<sys/types.h>
#include<sys/stat.h>
#pragma comment(lib,"WS2_32.lib")
#define PRINTF(str) printf("[%s - %d]"#str"%s",__func__,__LINE__,str);



//��ӡ������־
void error_die(const char* str) {
	perror(str);
	exit(1);

}

//��ָ���Ŀͻ����׽��ֶ�ȡһ�����ݣ����ֵ�buff�У�����ʵ�ʶ�ȡ�����ֽ���
int get_line(int sock, char* buff, int size) {
	char c = 0;
	int i = 0;
	while (i < size - 1 && c != '\n') {
		int n = recv(sock, &c, 1, 0);
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n') {
					recv(sock, &c, 1, 0);
				}
				else {

					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else {

			c = '\n';
		}
	}
	buff[i] = 0;
	return 0;
}

void unimplement(int client) {
	//��ָ���׽��֣�����һ��δ֧����ʾ��û��ʵ�ֵĴ���ҳ��


}

void not_found(int client) {
	//��ָ���׽��֣�����һ��δ֧����ʾ��û��ʵ�ֵĴ���ҳ��


}

void headers(int client) {
	//������Ӧ��ͷ��Ϣ
	char buff[1024];
	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);


	strcpy(buff, "Server:MyHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);


	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

}

void cat(int client,FILE* resource) {
	char buff[4096];
	int count = 0;
	while (1) {
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("total send [%d] to client\n",count);
}

void server_file(int client,const char* fileName) {
	char numchars = 1;
	char buff[1024];
	while (numchars > 0 && strcmp(buff, "/n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	FILE*  resource = fopen(fileName,"r");

	if (resource==NULL) {
		not_found(client);
	}
	else {
		//����ͷ��Ϣ
		headers(client);
		//�����ļ�
		cat(client, resource);

		printf("file send success");

	}
	fclose(resource);

}



//�����û�����
DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];
	int client = (SOCKET)arg;
	//1����ȡ��һ��
	int numchars = get_line(client, buff,sizeof(buff));
	PRINTF(buff);
	
	char method[255];
	int j = 0 ,i =0;
	while (!isspace(buff[j])&&i < sizeof(method)-1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);


	//2��������󷽷��Ƿ�֧��
	if (stricmp(method,"GET")&& stricmp(method, "POST")) {
		//����������ش�����ʾҳ��
		unimplement(client);
		return 0;
	}


	//3��������Դ·��
	char url[255];
	i = 0;
	while (isspace(buff[j]) && j < sizeof(buff)) {
		j++;
	}
	while (!isspace(buff[j])&& sizeof(url)-1 && j < sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);

	char path[512] = "";
	sprintf(path, "htdocs%s", url);
	if (path[strlen(path)-1]=='/') {
		strcat(path, "index.html");

	}
	PRINTF(path);

	struct stat status ;
	if (stat(path,&status)==-1) {
		//���������Ķ�������
		while (numchars>0&&strcmp(buff,"/n")) {
			numchars = get_line(client, buff, sizeof(buff));
		}
		PRINTF(buff);
		not_found(client);
	}else {
		if ((status.st_mode & S_IFMT)==S_IFDIR) {
			strcat(path, "index.html");
		}
		server_file(client,path);
	}

	closesocket(client);

	return 0;
}





int startup(unsigned short *port) {

	//1������ͨѶ��ʼ��
	WSADATA data;
	int res = WSAStartup(MAKEWORD(1,1), &data);
	if (res) {
		error_die("init fail");
	}
	//2�������׽���
	int server_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (server_socket == -1) {
		error_die("sock create fail");
	}

	//3���󶨶˿�
	int opt = 1;
	res = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*) & opt, sizeof(opt));

	if (res) {
		error_die("port bing fail");
	}

	//4�����׽���
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	res = bind(server_socket,(struct sockaddr*) &server_addr, sizeof(server_addr));

	if (res<0) {
		error_die("sock bing fail");
	}

	//5��������������
	int nameLen = sizeof(server_addr);
	if (*port == 0) {
		res = getsockname(server_socket, (struct sockaddr*)&server_addr,&nameLen);
		if (res) {
			error_die("dynamic sock create fail");
		}

		*port = server_addr.sin_port;
	}


	res = listen(server_socket, 5);
	if (res < 0) {
		error_die("listen queque create fail");
	}
	return server_socket;

};

int main(void) {
	//1����ʼ��
	unsigned short port = 8000;
	int server_sock = startup(&port);
	printf("http have benn started ,listening [%d] port...",port);




	//2������ʽ�ȴ��û�ͨ��������������
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	while (1) {
		int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1) {
			error_die("recevie user request fail");
		}
		//3�������µ��߳�ִ������
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, (void*)client_sock, 0, &threadId);
	}
	closesocket(server_sock);
	


	return 0;
}