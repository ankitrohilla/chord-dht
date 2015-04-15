/*
 * Author : Vaishali Gupta - 2014MCS2146
 *          Ankit Rohilla  - 2014MCS2118
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "threadc.h"
#include <sched.h>
#include <openssl/evp.h>
#include <variable.h>
#include <clienf.h>
#include <servef.h>

void client_thread();
void server_thread();
void command_try();

void _quit()
{
    int quit_sock;
    if((quit_sock = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      printf("\n Error : Could not create socket \n");
      //return 1;
    }

    memset( client_message, 0, sizeof(client_message));
    strcpy( client_message, "shut_this_ring");

    // NOW, send client_message to the successor
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    int p = atoi( current->succ_port );
    serv_addr.sin_port = htons(p);
    serv_addr.sin_addr.s_addr = inet_addr( current->succ_ip );

    if(connect(quit_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
        printf("Error : Connection Failed");
//            puts("SENDING");
//            puts(client_message);
    if( send(quit_sock , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }
    exit(1);

    close(quit_sock);
}

char* get_my_ip()
{
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&tmp);
    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && (!strcmp(tmp->ifa_name, "eth0") || !strcmp(tmp->ifa_name, "wlan0")) )
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            return inet_ntoa(pAddr->sin_addr);
        }
        tmp = tmp->ifa_next;
    }
    memset( loop_back, 0, sizeof(loop_back));
    strcpy( loop_back, "127.0.0.1");
    return loop_back;
}


int sha_hash_id(char *ip,char *port)
{
    int hash_id , sum = 0;

      EVP_MD_CTX mdctx;
      const EVP_MD *md;
      char data1[22];
      memset(data1, 0, sizeof(data1));
      strcpy(data1,ip);
      char data2[7];
      memset(data2, 0, sizeof(data2));
      strcpy(data2, port);
      strcat(data1,data2);

      unsigned char md_value[EVP_MAX_MD_SIZE];
      int md_len, i;

      OpenSSL_add_all_digests();

      md = EVP_get_digestbyname("md5");


      EVP_MD_CTX_init(&mdctx);
      EVP_DigestInit_ex(&mdctx, md, NULL);
      EVP_DigestUpdate(&mdctx, data1, strlen(data1));
      EVP_DigestFinal_ex(&mdctx, md_value, (unsigned int*)&md_len);
      EVP_MD_CTX_cleanup(&mdctx);

      for(i=0; i<md_len; i++) {
        sum += md_value[i];

    }

    hash_id = sum % M;

    return hash_id;
}


void client_thread()
{

    while(1)
    {

      if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
      {
        printf("\n-- Error : Could not create socket \n");
        //return 1;
      }

      memset(remote_ip, 0 , 15);
//    fgets(remote_ip, 40, stdin);

      memset(remote_port, 0 , 7);
//    fgets(remote_port, 40, stdin);

      memset(client_message, 0 , 40);
//    fgets(client_message, 40, stdin);


      if( !strcmp(command,"join") )
      {
          if( !connected_to_ring )
          {
              connected_to_ring = true;

              close(sockfd);

              // retreives the successor
              puts("\n-- Attempting to join the ring . . . . . .");
              _join();

              if( !connect_failed )
              {
                  for( int i = 0; i < N; i++ )
                  {
                       current->ftable.index[i] = (unsigned long int)( my_id + pow(2,i) ) % M;
                  }


                  //usleep(100000);
                  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
                  {
                    printf("\n-- Error : Could not create socket \n");
                    //return 1;
                  }

                  // notifies the successor to update its predecessor
                  puts("-- Notifing the successor of this new node . . . . . .");
                  _notify_successor();

                  // fixes finger table because there is a change in the network topology
                  puts("-- Initializing finger table . . . . . .");
                  _fix_my_finger_table();

                  finger_thread_id = create(_fix_my_finger_table_thread);
                  run(finger_thread_id);

                  puts("-- This node has successfully joined the ring");
              }
              else
              {
                 connect_failed = false;
                 connected_to_ring = false;
              }
          }
          else if(connected_to_ring)
              puts("-- You are already connected to ring.");
      } else if( !strcmp(command,"create") )
      {
         if( !connected_to_ring )
          {
              connected_to_ring = true;
             _create();

              // creating thread to fix finger table on regular intervals ...

              finger_thread_id = create(_fix_my_finger_table_thread);
              run(finger_thread_id);

          }
          else if( connected_to_ring )
              puts("-- You are already connected to ring");

      }else if(!strcmp(command,"port"))
      {
               if( connected_to_ring )
               {
                   puts("-- This node is already member of a ring and port cannot be changed\n-- Please restart this node to change port");
               }
      }else if( !strcmp(command,"dump"))
      {
          if(connected_to_ring)
          {
              puts("-- INFORMATION AT THIS NODE :");
              printf("-- My IP Address              - ");
              puts(current -> my_ip);
              printf("-- Successor's IP Address     - ");
              puts(current -> succ_ip);
              printf("-- Predecessor's IP Address   - ");
              puts(current -> pred_ip);
              printf("-- My port number             - ");
              puts(current -> my_port);
              printf("-- Successor's' port number   - ");
              puts(current -> succ_port);
              printf("-- Predecessor's' port number - ");
              puts(current -> pred_port);
              printf("-- My node ID                 - %d\n", current -> my_id   );

              printf("-- Successor's node ID        - %d\n", current -> succ_id );

              printf("-- Predecessor's'node ID      - %d\n", current -> pred_id );

              puts("\n-- Finger table of this node :\n");
              puts("    |------------------------------------------------------------------|");
              puts("    |    Index    |   Node ID  |  Node IP Address   | Node port number |");
              puts("    |-------------|------------|--------------------|------------------|");
              for(int i = 0; i < N; i++)
              {
                  printf("    |%7lu      |%7lu     |%15s     |  %10s      |\n", current->ftable.index[i], current->ftable.id[i], current->ftable.ip[i], current->ftable.port[i] );
              }
              puts("    |------------------------------------------------------------------|");

              puts("\n-- Data store of this node :\n");

              if( data_store.size() != 0 )
              {
                  typedef std::map<int, struct data_element>::const_iterator MapIterator;

                  printf("-- %ld data entries found\n\n", data_store.size());
                  puts("    ---------------------------------------------------------------------------------------");
                  puts("       Key value   |   Hash of key  |                      Data value                      ");
                  puts("    ---------------------------------------------------------------------------------------");
                  for (MapIterator iter = data_store.begin(); iter != data_store.end(); iter++)
                  {
                      printf("    %10s     | %10d     |  %s\n", data_store[ iter->first ].key_str, data_store[ iter->first ].hashed_key, data_store[ iter->first ].data_value );
                  }
                  puts("    ---------------------------------------------------------------------------------------");
              }
              else
                  puts("-- No Data Found !!!");
          }
          else
              puts("-- You are not connected to any ring");
      } else if( !strcmp(command,"dumpaddr") )
      {
          if(connected_to_ring)
          {
              puts("-- Attempting to retrieve Data Store of the node requested . . . . . .");
              close(sockfd);
              memset(remote_ip , 0 , sizeof(remote_ip));
              memset(remote_ip , 0 , sizeof(remote_port));

              scanf("%s" , c_remote_ip);
              scanf("%s" , c_remote_port);
              _dump_addr();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }

      } else if( !strcmp(command,"dumpall") )
      {
          if(connected_to_ring)
          {
              close(sockfd);
              _dump_all();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }

      } else if( !strcmp(command,"finger") )
      {
          if( !connected_to_ring )
          {
              puts("-- You are not connected to any ring");
          }
          else
          {
              close(sockfd);
              _finger();
          }

      }else if( !strcmp(command,"fingeraddr"))
      {
          if(connected_to_ring)
          {
              puts("-- Attempting to retrieve Finger Table of the node requested . . . . . .");
              close(sockfd);
              memset(remote_ip , 0 , sizeof(remote_ip));
              memset(remote_ip , 0 , sizeof(remote_port));

              scanf("%s" , c_remote_ip);
              scanf("%s" , c_remote_port);
              _finger_addr();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }

      }else if( !strcmp(command,"fingerall"))
      {
          if(connected_to_ring)
          {
              close(sockfd);
              _finger_all();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }

      }else if( !strcmp(command,"put") )
      {
          if(connected_to_ring)
          {
              close(sockfd);
              _put_data();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }
      } else if( !strcmp(command,"get"))
      {
          if(connected_to_ring)
          {
              close(sockfd);
              _get_data();
          }
          else
          {
              puts("-- You are not connected to any ring");
          }
      }
      else if( !strcmp(command,"quit"))
      {
          puts("-- Shutting down the ring");
          _quit();
      }
      else if( !strcmp(command,"successor"))
      {
          if(connected_to_ring)
             printf("\n-- Successor Node Address  -  %s : %s\n",current->succ_ip , current->succ_port);
          else
          {
              puts("-- You are not connected to any ring");
          }
      }
      else if( !strcmp(command,"predecessor"))
      {
          if(connected_to_ring)
             printf("\n-- Predecessor Node Address  -  %s : %s\n",current->pred_ip , current->pred_port);
          else
          {
              puts("-- You are not connected to any ring");
          }
      }
      else if( !strcmp(command,"help"))
      {
          puts("COMMANDS SUPPORTED (syntax) - ");
          puts("\n a)  ' help ' \n  -- Provides a list of command and their usage details");
          puts(" b)  ' port <x> ' \n  -- Listen on this port for other instances of the program over different nodes (If not provided then default port '10000' is used) ");
          puts(" c)  ' create ' \n  -- Creates the ring");
          puts(" d)  ' join <ipaddress> <port> ' \n  -- Join the ring with  IP address <ipaddress> and port number <port>");
          puts(" e)  ' quit ' \n  -- Shuts down the ring");
          puts(" f)  ' put <key> <value> ' \n  -- Insert the given <key,value> pair in the ring");
          puts(" g)  ' get <key> ' \n  -- Returns the value corresponding to the key, if one was previously inserted in the node");
          puts(" h)  ' successor ' \n  -- Gives address of the next node on the ring");
          puts(" i)  ' predecessor ' \n  -- Gives address of the previous node on the ring");
          puts(" j)  ' dump ' \n  -- Display all information (finger table , data stored ,etc ..) pertaining to calling node");
          puts(" k)  ' dumpaddr <ipaddress> <port> ' \n  -- Display Data Store pertaining to node at address <ipaddress> : <port>");
          puts(" l)  ' dumpall ' \n  -- Display Data Store of all the nodes ( including calling node )");
          puts(" m)  ' fingeraddr <ipaddress> <port> ' \n  -- Display Finger Table pertaining to node at address <ipaddress> : <port>");
          puts(" n)  ' fingerall ' \n  -- Display Finger Table of all the nodes ( including calling node )");
          puts(" o)  ' finger ' \n  -- A list of addresses of nodes on the ring");

          puts("\nPOINTS TO REMEMBER - \n");
          puts(" ~ Once a port is set , it cannot be changed");
          puts(" ~ Once a ring is created by particular node , node cannot be joined to another ring");
          puts(" ~ Once a node is joined to a ring , it cannot be joined to another ring or create a new ring");

      }
      else
          puts("-- Command unknown\n-- Please type 'help' to know about supported commands");

      close(sockfd);
      //sleep(1);
      printf("\n > ");
      fflush(stdout);
      scanf("%s", command);
    }
 // return 0;
}

void server_thread( )
{
  int listenfd = 0 , n = 0 ;

  struct sockaddr_in serv_addr;
 
  port = (unsigned short) atoi(my_port);

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  int true1 = 1;
  setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &true1, sizeof(int));

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;    
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
  serv_addr.sin_port = htons(port);

  if( bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0 )
  {
      puts("\n-- Trying to connect . . . . . .\n-- Unable to bind, please try another port");
      fflush(stdout);
      unable_bind = true;
      //deleteThread(getID());
  }
  else
  {
    unable_bind = false;
    int id = create(client_thread);
    run(id);
  }

  while(unable_bind)
  {

      command_try();

      if( !strcmp(command,"port") || !strcmp(command,"create") || strcmp(command,"join"))
      {

          port = (unsigned short) atoi(my_port);
          listenfd = socket(AF_INET, SOCK_STREAM, 0);
          int true1 = 1;
          setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &true1, sizeof(int));

          memset(&serv_addr, 0, sizeof(serv_addr));

          serv_addr.sin_family = AF_INET;
          serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
          serv_addr.sin_port = htons(port);

          if( bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0 )
          {
              puts("\n-- Trying to connect . . . . . .\n-- Unable to bind, please try another port");
              fflush(stdout);
              unable_bind = true;
              //deleteThread(getID());
          }
          else
          {
            unable_bind = false;
            int id = create(client_thread);
            run(id);
          }
      }
  }



  if(listen(listenfd, 10) == -1){
      printf("Failed to listen\n");
      return;
  }

  while(1)
    {
      connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request

      memset(server_received, 0, sizeof(server_received));

      n=recv(connfd, server_received, sizeof(server_received), 0);

      char temp[15];

      for(int i=0;i<14;i++)
         temp[i] = server_received[i];
      temp[14] = 0;

      memset(server_message, 0, sizeof(server_message));

      // server_received has the format find_successor <IPA> <PORT><WHITESPACE><WHITESPACE>
      if( !strcmp(temp, "find_successor") )
      {
//        puts("find_successor");
          strcpy( server_message , _find_successor() );
      }
      else if( !strcmp(temp, "updpredecessor") )
      {
          printf("\n-- Update predecessor command received\n-- Updating . . . . . .\n");
          fflush(stdout);
          _update_predecessor();
          printf("-- Updation done\n > ");
          fflush(stdout);
      }
      else if( !strcmp(temp, "return_f_entry"))
      {
//        puts("return_f_entry");
          strcpy( server_message , _check_table() );
      }
      else if( !strcmp(temp, "str_data_entry"))
      {
          _store_data_entry();
          printf("-- Data entry stored\n > ");
          fflush(stdout);
      }
      else if( !strcmp(temp , "get_data_entry"))
      {
          printf("\n-- Message received - '%s'\n-- Retreiving data pertaining to provided key . . . . . .\n", server_received);
          fflush(stdout);
          strcpy(server_message , _get_data_entry());
          printf("-- Retreived data entry sent\n > ");
          fflush(stdout);
      }else if( !strcmp(temp , "shut_this_ring"))
      {
          puts("\n-- Shutting down the ring");
          _quit();
      }else if( !strcmp(temp , "give_your_dump"))
      {
          printf("\n-- Dump command received\n-- Providing data information . . . . . .\n");
          fflush(stdout);
          _give_dump();
          printf("-- This node's data information sent\n > ");
          fflush(stdout);
      }else if( !strcmp(temp , "give_your_info"))
      {
          printf("\n-- Finger command received\n-- Providing node information . . . . . .\n");
          fflush(stdout);
          memset(server_message,0,sizeof(server_message));
          strcpy(server_message,current->succ_ip);
          strcat(server_message," ");
          strcat(server_message,current->succ_port);
          strcat(server_message,"  ");
          printf("-- This node successor's information sent\n > ");
          fflush(stdout);

      }else if( !strcmp(temp , "gve_finger_tbl"))
      {
          printf("\n-- Finger Table command received\n-- Providing Finger Table information . . . . . .\n");
          fflush(stdout);
          _give_finger_table();
          printf("-- This node's finger table information sent\n > ");
          fflush(stdout);
      }

      write(connfd, server_message, strlen(server_message));
      close(connfd);
    }

}

//to test commands until bind is successful

void command_try()
{
    printf("\n > ");
    fflush(stdout);
    memset(command,0,sizeof(command));
    scanf("%s", command);

    if(!strcmp(command,"port"))
    {
             if( connected_to_ring )
             {
                 puts("-- This node is already member of a ring and port cannot be changed\n-- Please restart this node to change port");
             }
             else
             {
                 memset(my_port, 0 , sizeof(my_port));
                 scanf("%s",my_port);
                 memset(try_port, 0 , sizeof(try_port));
                 strcpy(try_port,my_port);
                 my_id = sha_hash_id(my_ip , my_port);
                 for( int i = 0; i < N; i++ )
                 {
                      current->ftable.index[i] = (unsigned long int)( my_id + pow(2,i) ) % M;
                 }
                 printf("-- Port has been set to %s\n", my_port);
             }
    }
    else if( !strcmp(command,"create") || ( !strcmp(command ,"join")))
    {
        memset(my_port, 0 , sizeof(my_port));
        strcpy(my_port , "10000");
        puts("-- Default port 10000 has been used");

    }

    else if( !strcmp(command,"help"))
    {
        puts("COMMANDS SUPPORTED (syntax) - ");
        puts("\n a)  ' help ' \n  -- Provides a list of command and their usage details");
        puts(" b)  ' port <x> ' \n  -- Listen on this port for other instances of the program over different nodes (If not provided then default port '10000' is used) ");
        puts(" c)  ' create ' \n  -- Creates the ring");
        puts(" d)  ' join <ipaddress> <port> ' \n  -- Join the ring with  IP address <ipaddress> and port number <port>");
        puts(" e)  ' quit ' \n  -- Shuts down the ring");
        puts(" f)  ' put <key> <value> ' \n  -- Insert the given <key,value> pair in the ring");
        puts(" g)  ' get <key> ' \n  -- Returns the value corresponding to the key, if one was previously inserted in the node");
        puts(" h)  ' successor ' \n  -- Gives address of the next node on the ring");
        puts(" i)  ' predecessor ' \n  -- Gives address of the previous node on the ring");
        puts(" j)  ' dump ' \n  -- Display all information (finger table , data stored ,etc ..) pertaining to calling node");
        puts(" k)  ' dumpaddr <ipaddress> <port> ' \n  -- Display Data Store pertaining to node at address <ipaddress> : <port>");
        puts(" l)  ' dumpall ' \n  -- Display Data Store of all the nodes ( including calling node )");
        puts(" m)  ' fingeraddr <ipaddress> <port> ' \n  -- Display Finger Table pertaining to node at address <ipaddress> : <port>");
        puts(" n)  ' fingerall ' \n  -- Display Finger Table of all the nodes ( including calling node )");
        puts(" o)  ' finger ' \n  -- A list of addresses of nodes on the ring");

        puts("\nPOINTS TO REMEMBER - \n");
        puts(" ~ Once a port is set , it cannot be changed");
        puts(" ~ Once a ring is created by particular node , node cannot be joined to another ring");
        puts(" ~ Once a node is joined to a ring , it cannot be joined to another ring or create a new ring");

    }

    else if( !strcmp(command,"dumpaddr") )
         {
             if(connected_to_ring)
             {
                 puts("-- Attempting to retrieve Data Store of the node requested . . . . . .");
                 close(sockfd);
                 memset(remote_ip , 0 , sizeof(remote_ip));
                 memset(remote_ip , 0 , sizeof(remote_port));

                 scanf("%s" , c_remote_ip);
                 scanf("%s" , c_remote_port);
                 _dump_addr();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }

         } else if( !strcmp(command,"dumpall") )
         {
             if(connected_to_ring)
             {
                 close(sockfd);
                 _dump_all();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }

         } else if( !strcmp(command,"finger") )
         {
             if( !connected_to_ring )
             {
                 puts("-- You are not connected to any ring");
             }
             else
             {
                 close(sockfd);
                 _finger();
             }

         }else if( !strcmp(command,"fingeraddr"))
         {
             if(connected_to_ring)
             {
                 puts("-- Attempting to retrieve Finger Table of the node requested . . . . . .");
                 close(sockfd);
                 memset(remote_ip , 0 , sizeof(remote_ip));
                 memset(remote_ip , 0 , sizeof(remote_port));

                 scanf("%s" , c_remote_ip);
                 scanf("%s" , c_remote_port);
                 _finger_addr();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }

         }else if( !strcmp(command,"fingerall"))
         {
             if(connected_to_ring)
             {
                 close(sockfd);
                 _finger_all();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }

         }else if( !strcmp(command,"put") )
         {
             if(connected_to_ring)
             {
                 close(sockfd);
                 _put_data();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }
         } else if( !strcmp(command,"get"))
         {
             if(connected_to_ring)
             {
                 close(sockfd);
                 _get_data();
             }
             else
             {
                 puts("-- You are not connected to any ring");
             }
         }
         else if( !strcmp(command,"quit"))
         {
             puts("-- Shutting down the ring");
             _quit();
         }
         else if( !strcmp(command,"successor"))
         {
             if(connected_to_ring)
                printf("\n-- Successor Node Address  -  %s : %s\n",current->succ_ip , current->succ_port);
             else
             {
                 puts("-- You are not connected to any ring");
             }
         }
         else if( !strcmp(command,"predecessor"))
         {
             if(connected_to_ring)
                printf("\n-- Predecessor Node Address  -  %s : %s\n",current->pred_ip , current->pred_port);
             else
             {
                 puts("-- You are not connected to any ring");
             }
         }
         else if( !strcmp(command,"dump"))
        {
            if( !connected_to_ring)
                puts("-- You are not connected to any ring");
        }
        else
            puts("-- Command unknown\n-- Please type 'help' to know about supported commands");

    if( !strcmp(command,"port") || !(command,"join") || !(command,"create"))
        my_id = sha_hash_id(my_ip , my_port);



}


int main(int argc,char **argv)
{
    memset(current -> my_ip,     0, 15);
    memset(current -> succ_ip,   0, 15);
    memset(current -> pred_ip,   0, 15);
    memset(current -> my_port,   0,  7);
    memset(current -> succ_port, 0,  7);
    memset(current -> succ_port, 0,  7);


    printf("\n--------------------------------> Type 'help' for any information  <----------------------------------  \n");

    strcpy(my_port, "-----");

    printf("-- Obtaining IP Address\n");
    struct hostent *HostEntPtr;
    struct in_addr in;
    char Hostname[100];

gethostname( Hostname, sizeof(Hostname) );
HostEntPtr = gethostbyname( Hostname );

if ( HostEntPtr != NULL )
{
  memcpy( &in.s_addr, *HostEntPtr->h_addr_list, sizeof(in.s_addr) );
  strcpy(my_ip , get_my_ip());
  printf( "ip address is  %s\n", my_ip );
}

command_try();

while( strcmp(command,"port") && strcmp(command,"create") && strcmp(command,"join"))
{
    command_try();
}


printf("\n-- Running client and server \n");



 /* int server_port=atoi(argv[1]);
  int client_port=atoi(argv[2]);*/
  //strcpy(s,argv[3]);
 

 //if (pid==0)
  //pthread_create(&thread,NULL,server,NULL);
// else
 // pthread_create(&thread1,NULL,client,NULL);
server_id = create(server_thread);
start();

  //start();
 
 return 0;
}
