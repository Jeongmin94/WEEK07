int open_clientfd(char *hostname, char *port) : Establish a connection with a server. / client side

int open_listenfd(char *port) : Create a listening descriptor that can be used to accept connection requests from clients. / server side

/* 도메인 주소를 받아서 네트워크 주소 정보(IP address)를 가져오는 함수
hostname, service, hints는 입력 매개변수이고 result는 출력 매개변수
hostname : 호스트 이름, 주소 문자열(IPv4 or IPv6)
service : 서비스 이름, 포트 번호 문자열
hints : getaddrinfo 함수에게 희망하는 리턴값의 유형을 알려줌. addrinfo 구조체에 힌트를 채워줌
result : DNS 서버로부터 받은 네트워크 주소 정보를 돌려줌. addrinfo 링크드 리스트의 시작주소가 들어옴
*/
int getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result);

/* getaddrinfo가 return한 값을 사람이 읽을 수 있는 문자열로 변환해줌*/
const char *gai_strerror(int errcode);

/* 네트워크 주소 정보 받아서 도메인 주소 정보로 변환하는 함수 
sa : 도메인 주소로 변환하고 싶은 네트워크 주소 정보(소켓)를 가진 구조체
salen : sa 구조체가 가진 길이의 주소
host : hostlen 바이트 길이를 가지고 있을 host 버퍼
service : servlen 바이트 길이를 가지고 있을 service 버퍼
IP주소를 가진 소켓 sa를 호스트 혹은 서비스 이름으로 변환하고 이를 host와 service 버퍼로 복사한다.
호스트이름을 쓰기 싫으면 호스트를 NULL hostlen을 0으로, 서비스 이름을 쓰기 싫으면 서비스를 NULL servlen을 0으로 하면 된다. 둘 중 하나는 써야된다.
flags => NI_NUMERICHOST / NI_NEMERICSERV
*/
int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *service, size_t servlen, int flags);

/* 소켓 정보 설정 */
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

/* robust i/o */
typedef struct {
	int rio_fd;
	int rio_cnt;
	char *rio_bufptr;
	char rio_buf[RIO_BUFSIZE];
} rio_t;
/* rio_t 구조체 초기화 */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  /* unread size 라고는 하는데 사용하는 걸 보면 read size 라고 봐야 된다.... */
    rp->rio_bufptr = rp->rio_buf; /* 내부 버퍼 포인터 */
}

// rio_read
// rio_t 구조체 내부의 버퍼를 사용.
// 내부 버퍼에 저장된 것을 저장할 버퍼에 복사.
// rio_readn은 지정된 크기만큼 읽는 것이 보장되지만, rio_rean는 보장되지 않는다.

/* 아래부터 책 내용 정리
------------------------------------------------------------------------------------
*/

// 소켓 인터페이스

/* 소켓 생성
클라이언트와 서버는 소켓 식별자를 생성하기 위해 socket함수를 사용한다
socket에 의해 리턴된 clientfd 식별자는 부분적으로 열린 것이기 때문에, 아직 읽거나 쓸 수 없다.
소켓의 오픈 과정은 클라이언트인지 서버인지에 따라 다르다.
*/
int socket(int domain, int type, int protocol);

// 클라이언트 사이드 - connect
/* 클라이언트는 connect 함수를 호출해서 서버와의 연결을 수행한다.
소켓 주소 addr의 서버와 인터넷 연결을 시도하게 된다.
clientfd 식별자가 읽거나 쓸 준비가 되면 다음과 같은 소켓 쌍으로 규정된다.
(x:y, addr.sin_addr:addr.sin_port)
x는 클라이언트의 ip 주소이며, y는 호스트의 클라이언트 프로세스를 유일하게 식별하는 단기 포트이다.
*/
int connect (int clientfd, const struct sockaddr *addr, socklen_t addrlen);


// 서버 사이드 - bind, listen, accept
/*
서버는 socket() 함수를 통해 생성된 응용 프로그램 내에서 유일한 번호를 가진 소켓을 생성하게 된다.
이 소켓의 번호는 응용 프로그램만 알고 사용하는 번호이기 때문에 외부와 통신하기 위해 소켓 번호와 TCP/IP 서버 시스템이 제공하는 소켓 주소(IP주소 + 포트번호)를 연결해두어야 한다. 그래서 bind()를 사용한다.
bind()는 응용 프로그램 자신의 주소와 소켓 번호를 연결하는 작업이다.
서버에서 bind()가 반드시 필요한 이유는 임의의 클라이언트가 서버의 특정 프로그램이 만든 소켓과 통신을 하려면 그 소켓을 찾아야 하기 때문이다.
따라서 자신의 소켓 번호와 클라이언트가 알고 있을 서버의 IP 주소 및 포트 번호를 미리 연결시켜야 하는 것이다.
*/
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// TCP 통신 절차 -> listen(), accept()의 호출이 필요하다.
/*
서버는 클라이언트로부터의 연결 요청을 받아들이기 위해 이를 기다리고 있어야 하는데, 이를 위해 listen()을 사용해야 한다.
listen()은 소켓을 듣기 소켓 모드로 변환시키는 작업이므로 즉시 리턴된다.
backlog는 서버에서 accept()를 처리하는 동안 대기시킬 수 있는 클라이언트의 최대 connect() 요청 수이다.
*/
int listen(int sockfd, int backlog);

/*
서버가 listen을 리턴한 뒤에 어떤 클라이언트에서 connect로 이 서버에 연결요청을 보내오면 이를 처리하기 위해 서버는 accept를 호출해두어야 한다.
accept()에 성공하면 connect를 요청한 클라이언트와 1:1 통신에 사용할 새로운 소켓이 만들어지고, accept는 새롭게 만들어진 소켓번호를 리턴한다.
*/
int accept(int listenfd, struct sockaddr *addr, int *addrlen);

/*
close()는 소켓을 닫을 때 호출한다. UDP에서 close를 호출하면 단순히 사용하던 소켓을 닫는 작업만 수행한다.
하지만 TCP 소켓은 연결형 서비스이므로 현재 미처리된 패킷들을 모두 처리한 후에 소켓을 닫게 된다.
setsockopt() 함수를 사용하면 미처리 패킷을 즉시 모두 버리게 한거나, 지정한 시간동안 처리되기를 기다릴 수 있다.
*/

// 호스트와 서비스의 변환
/*
리눅스에는 getaddrinfo와 getnameinfo라는 함수를 통해 소켓의 주소 정보를 변환할 수 있게 해준다.
getaddrinfo함수는 호스트의 이름/주소, 서비스의 이름/포트번호를 IP 통신에 필요한 소켓 주소 구조체로 변환해준다.
host와 service가 주어지면 getaddrinfo가 host와 service에 대응되는 소켓 주소 구조체의 연결 리스트를 가리키는 result를 반환한다.

- 클라이언트에서 getaddrinfo를 호출 할 때-
클라이언트가 getaddrinfo 함수를 호출하면, result에 담겨있는 소켓 주소 구조체 리스트를 탐색한다.
그리고 소켓의 각 주소에 대해 connect 함수를 호출하면서 연결이 성공할 때까지 반복한다.

- 서버에서 getaddrinfo를 호출 할 때-
서버가 getaddrinfo 함수를 호출하면 result에 담겨있는 소켓 주소 구조체 리스트를 탐색한다.
그리고 소켓의 각 주소에 대해 bind 함수를 호출하면서 연결이 성공할 때까지 반복한다.

host에는 도메인 이름이나 숫자주소가, service에는 서비스 이름(ex. http..)이나 포트번호가 들어갈 수 있다.
host나 service 이름을 주소로 변환하고 싶지 않다면 NULL을 사용하면 된다.(한 개는 명시 필수)
hints는 탐색할 소켓을 한정지어 주는데, ai_family, ai_socktype, ai_protocol, ai_flags만 설정 가능하다.(addrinfo 구조체에 보다 많은 정보가 있음)
*/

int getaddrinfo(const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result);

/*
getnameinfo 함수는 getaddrinfo의 반대 역할을 한다. 소켓 주소 구조체에 저장되어 있는 IP 통신에 필요한 소켓 주소를 대응되는 host와 service 이름으로 변환해준다.
sa는 길이가 salen인 소켓 주소 구조체를, host와 service는 각각 길이가 hostlen, servlen인 호스트와 서비스 버퍼를 가리킨다.
getnameinfo 함수는 sa의 주소와 대응되는 호스트와 서비스 이름을 문자열로 변환하여 host와 service 버퍼에 저장한다.
호스트나 서비스 이름을 받고 싶지 않다면 NULL, 길이를 0으로 입력하면 된다.
*/

int getnameinfo(const struct sockaddr *as, socklen_t salen, char *host, size_t hostlen, char *service, size_t servlen, int flags);


/*
open_clientfd : 클라이언트는 open_client 함수를 호출해서 서버와 연결을 설정할 수 있다.
open_clientfd 함수는 호스트 hostname에서 실행되고, 포트번호 port에서 연결 요청을 기다리는 서버에 연결을 시도한다.
*/

int open_clientfd(char *hostname, char *port);

/*
open_listenfd : 서버는 open_listenfd 함수를 호출해서 연결 요청을 받을 준비가 된 듣기 소켓을 생성한다.
open_listenfd 함수는 포트 port에서 듣기 준비가 완료된 듣기 소켓을 리턴한다.
*/

int open_listenfd(char *port);


// Tiny 웹 서버 작동 순서
/*
메인 루틴에서 듣기 소켓을 생성하고 클라이언트의 요청이 들어올 때 까지 기다림
클라이언트가 접속하면 accept를 하고 클라이언트 getnameinfo하여 호스트, 포트 번호를 출력
accept를 통해 생성된 소켓을 doit 함수에 전달함.
doit 함수에서 전달받은 소켓의 request header를 읽고 서버 화면에 출력해줌.
서버에 처음 접속하면 method는 GET uri는 /, version은 HTTP/1.1이 들어옴.
*/

// 첫 번째 인자의 환경변수를 두 번째 인자로 바꿈
// 0일 경우 overwrite안됨
setenv("QUERY_STRING", cgiargs, 1);

// 두 번째 인자인 표준출력이 sockfd를 가리키게 변함
Dup2(sockfd, STDOUT_FILENO); 

// filename이라는 프로그램(함수, 경로 등이 될 수 있음)를
// 인자로 emptylist를 받아서 실행함
// environ은 설정된 환경 변수를 받아서 실행하겠다는 의미
Execve(filename, emptylist, environ);

/*
HTTP 헤더
HTTP 헤더는 클라이언트와 서버가 요청 또는 응답으로 부가적인 정보를 전송할 수 있도록 해줍니다. HTTP 헤더는 대소문자를 구분하지 않는 이름과 콜른 ':' 다음에 오는 값으로 이루어져 있습니다. 값 앞에 붙은 빈 문자열은 무시됩니다.

General header : 요청과 응답 모두에 적용되지만, 바디에서 최종적으로 전송되는 데이터와는 관련이 없는 헤더
Request Header : 페치될 리소스나 클라이언트 자체에 대한 자세한 정보를 포함한 헤더
Reponse Header : 위치 또는 서버 자체에 대한 정보(이름, 버전..)와 같이 응답에 대한 부가적인 정보를 갖는 헤더
*/

/*
Dynamic Content
<form action="/cgi-bin/adder" method="GET">
HTML 파일에서 action 태그를 사용해서 submit 타입의 버튼을 클릭했을 때, 해당 uri로 입력받은 값을 전달시킬 수 있다.
uri를 파싱하는 과정에서 동적 콘텐츠의 filename은 ? 이전까지의 uri주소가 저장되고 cgiargs에는 ? 이후의 uri주소가 저장된다.
이 후 cgiargs에 저장된 문자열이 환경 변수에 덮어 씌우고, 표준 출력 형식을 전달받은 sockfd로 돌린다.(클라이언트에게 데이터 전달하기 이함)
그리고 filename에는 서버에 저장되어 있는 프로그램의 경로가 입력되고
이 경로에 있는 프로그램을 실행할 때 emptylist를 인자로 받아 덮어 씌어진 환경 변수에서 프로그램을 실행시킨다.
*/

/*
HTML form 태그
사용자의 의견이나 정보를 알기 위해 입력할 틀을 만드는 데 사용된다.
form은 입력된 데이터를 한 번에 서버로 전송한다. 전송한 데이터는 웹 서버가 처리하고, 결과에 따른 또 다른 웹페이지를 보여준다. 
== form 태그 동작 방식 ==
1. form 태그가 있는 웹 페이지를 방문
2. form의 내용을 입력함(submit, txt..)
3. form 안에 있는 데이터를 웹 서버로 전송(현재 url 마지막에 ?가 붙고 지정한 id나 name에 맞는 값이 추가적으로 붙어서 전송됨)
4. 웹 서버는 받은 폼 데이터를 처리하기 위해 웹 프로그램으로 넘김(dynamic content)
5. 웹 프로그램은 폼 데이터를 처리
6. 처리 결과에 따른 새로운 html을 웹 서버에 전송
7. 웹 서버는 새로 받은 html을 브라우저에게 전송
8. 브라우저는 새로 받은 html을 보여줌
*/

/*
환경변수 설정(QUERY_STRING 등)
http://dpnm.postech.ac.kr/cs103/lab-material/11/cse103-1120-CGI.html
*/