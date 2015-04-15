// M is no. of nodes and N is no. of bits per ID
#define M 16
#define N 4

bool connected_to_ring = false, fix_data_store = false , connect_failed = false, put_occurred = false, unable_bind = true, unknown_command = false;

char command[30];

// in case machine is not connected to an external network
char loop_back[15];

struct sockaddr_in serv_addr, client;

int sockfd = 0, connfd = 0 , n = 0 , server_id = 0 , finger_thread_id = 0 ;

char client_message[60], server_message[60];

char server_received[60], client_received[60];

char my_ip[15],     my_port[7] ,  try_port[7];
char remote_ip[15], remote_port[7];
unsigned short port;

long int socket_desc, new_socket, c ,my_id, remote_id;

struct finger_table
{
    // succ_ip_address[i], port[i] contains IPA, port of node whose ID is (my_id + 2^i) mod 16

    unsigned long index[N];
    unsigned long id[N];
    char ip[N][15];
    char port[N][7];
};

struct node
{
    int my_id , pred_id , succ_id;
    char my_ip[15],  pred_ip[15] , succ_ip[15];
    char my_port[7], pred_port[7], succ_port[7];

    finger_table ftable;

}*current = new node();

struct data_element
{
    // key of data element to be hashed
    char key_str[ 5 ];

    // hash of key
    int hashed_key;
    char data_value[ 200 ];
};

std::map<int,struct data_element> data_store;
