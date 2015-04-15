/**********************  FIX_MY_FINGER_TABLE_THREAD  ***********************/
/**********************  FIX_MY_FINGER_TABLE         ***********************/
/**********************  FIX_MY_DATA_STORE           ***********************/
/**********************  CREATE                      ***********************/
/**********************  JOIN                        ***********************/
/**********************  NOTIFY_SUCCESSOR            ***********************/
/**********************  PUT_DATA                    ***********************/
/**********************  GET_DATA                    ***********************/
/**********************  DUMPADDR                    ***********************/
/**********************  DUMPALL                     ***********************/
/**********************  FINGER                      ***********************/
/**********************  FINGERADDR                  ***********************/
/**********************  FINGERALL                   ***********************/



int sha_hash_id(char *ip,char *port);
void _fix_my_finger_table();
void _fix_my_data_store();

// variables to handle data
char c_key_str[ 5 ];
char c_data_value[ 200 ];
int  c_hashed_key;
char c_hashed_key_str[ 5 ];
char c_remote_ip[ 15 ];
char c_remote_port[ 7 ];
char c_index[ 5 ];
char c_ip_entry[ 15 ];
char c_port_entry[ 7 ];

// used for _fix_my_finger_table_thread
char finger_update_message[60], finger_update_received[60];
char data_update_message[60], data_update_received[60];


// a separate socket for fix_my_finger table which will be used in a separate
// parallel thread
int local_sockfd , local_sockfd_2;

void _fix_my_finger_table_thread()
{
    while(1)
    {
        _fix_my_finger_table();

        // fix data store if my predecessor has been updated
        // the bool below is TRUE after _update_predecessor is called
        if( fix_data_store )
        {
            _fix_my_data_store();
            fix_data_store = false;
        }
    }

}

void _fix_my_finger_table()
{
    if((local_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      //printf("\n Error : Could not create socket \n");
      //return 1;
    }

    int iid, rid;

    for( int i = 0; i < N; i++ )
    {
        // ith index entry of finger table
        iid = current -> ftable.index[ i ];

        char iid_str[7];

        memset( iid_str, 0, sizeof(iid_str));
        sprintf(iid_str, "%d", iid);

        // finger_update_message is fixed, no matter where is the server, it sends its own information
        memset(finger_update_message,0,sizeof(finger_update_message));

        strcpy(finger_update_message,"return_f_entry");
        strcat(finger_update_message," ");
        strcat(finger_update_message,my_ip);
        strcat(finger_update_message," ");
        strcat(finger_update_message,my_port);
        strcat(finger_update_message," ");
        strcat(finger_update_message,iid_str);
        strcat(finger_update_message,"  ");

        memset(remote_ip, 0  , sizeof(remote_ip  ));
        memset(remote_port, 0, sizeof(remote_port));

        // initially, forward the query to my own server
        strcpy( remote_ip  , my_ip   );
        strcpy( remote_port, my_port );

        // forward the query to the node specified until correct node <IPA> and <PORT> are received
        char response[6];

        do
        {
            // I will send a message to server in EACH ITERATION
            if((local_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
            {
              //printf("\n-- Error : Could not create socket \n");
              //return 1;
            }

            // NOW, send finger_update_message to the changed remote_ip and remote_port and see if it can
            // provide me the correct node information
            memset(&serv_addr, 0, sizeof(serv_addr));

            serv_addr.sin_family = AF_INET;
            int p = atoi( remote_port );
            serv_addr.sin_port = htons(p);
            serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

            while(connect(local_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);

            if( send(local_sockfd , finger_update_message , sizeof(finger_update_message) , 0) < 0)
            {
                puts("Send failed");
            }

            memset(finger_update_received, 0, sizeof(finger_update_received));

            n = read(local_sockfd, finger_update_received, sizeof(finger_update_received));

            int c = 0;
            int n_holder = 0;
            memset( response   , 0, sizeof(response   ));
            memset( remote_ip  , 0, sizeof(remote_ip  ));
            memset( remote_port, 0, sizeof(remote_port));

            // finger_update_received has the format either forwd/found <IPA> <PORT><WHITESPACE><WHITESPACE>
            // where forwd means forward the request and found means <IPA> <PORT> is the required result
            // extract ip and port
            for( int m = 0; finger_update_received[m] != 32; m+=n_holder )
            {
                for( int n = 0; finger_update_received[m+n] != 32; n++ )
                {
                    switch (c)
                    {
                        case 0:
                        response[n]    = finger_update_received[m+n];
                            break;
                        case 1:
                        remote_ip[n]   = finger_update_received[m+n];
                            break;
                        case 2:
                        remote_port[n] = finger_update_received[m+n];
                            break;
                    }
                    n_holder = n;
                }
                switch(c)
                {
                case 0:
                    response [n_holder + 1]   = 0;
                case 1:
                    remote_ip[n_holder + 1]   = 0;
                    break;
                case 2:
                    remote_port[n_holder + 1] = 0;
                    break;
                }
                c++;
                n_holder += 2;
            }

            rid = sha_hash_id(remote_ip, remote_port);

            if( !strcmp( response, "found" ))
            {
                strcpy( current->ftable.ip[i]   , remote_ip   );
                strcpy( current->ftable.port[i] , remote_port );
                current->ftable.id[i] = rid;
            }
            else if( !strcmp( response, "forwd" ))
            {
                // forward the query
            }
            else
                puts("IMPOSSIBLE");

            close(local_sockfd);
        } while( !strcmp( response , "forwd" ) );
    }
}

void _fix_my_data_store()
{

    puts("\n-- Attempting to distribute data to new nodes");

    std::vector<int> key_del;

    // did is H(KEY) for each key updated in the loop below
    int pid = current->pred_id;
    int mid = current->my_id;
    int did;
    char hashed_key_str[5];

    memset(hashed_key_str, 0, sizeof(hashed_key_str));

    typedef std::map<int, struct data_element>::const_iterator MapIterator;
    for(MapIterator iter = data_store.begin(); iter != data_store.end(); iter++)
    {
        did = data_store[ iter->first ].hashed_key;

        sprintf( hashed_key_str, "%d", did);

        if( did <= pid && pid < mid || pid < mid && mid < did || mid < did && did <= pid )
        {
            //printf("in if condition");
            // data_update_message is fixed, no matter where is the server, it sends its own information
            memset(data_update_message, 0, sizeof(data_update_message));

            strcpy(data_update_message,"return_f_entry");
            strcat(data_update_message," ");
            strcat(data_update_message,my_ip);
            strcat(data_update_message," ");
            strcat(data_update_message,my_port);
            strcat(data_update_message," ");
            strcat(data_update_message,hashed_key_str);
            strcat(data_update_message,"  ");

            memset(remote_ip, 0  , sizeof(remote_ip  ));
            memset(remote_port, 0, sizeof(remote_port));

            // initially, forward the query to my own server
            strcpy( remote_ip  , my_ip   );
            strcpy( remote_port, my_port );

            char response[6];

            do {

                if((local_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
                {
                  printf("\n-- Error : Could not create socket \n");
                  //return 1;
                }

                memset( response, 0, sizeof( response) );

                // NOW, send data_update_message to the changed remote_ip and remote_port and see if it can
                // provide me the correct node information
                memset(&serv_addr, 0, sizeof(serv_addr));

                serv_addr.sin_family = AF_INET;
                int p = atoi( remote_port );
                serv_addr.sin_port = htons(p);
                serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

                while(connect(local_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);
                    //printf("Error : Connection Failed");
                if( send(local_sockfd , data_update_message , sizeof(data_update_message) , 0) < 0)
                {
                    puts("Send failed");
                }

                memset(data_update_received, 0, sizeof(data_update_received));

                n = read(local_sockfd, data_update_received, sizeof(data_update_received));

                int c = 0;
                int n_holder = 0;
                memset( response   , 0, sizeof(response   ));
                memset( remote_ip  , 0, sizeof(remote_ip  ));
                memset( remote_port, 0, sizeof(remote_port));

                // data_update_received has the format either forwd/found <IPA> <PORT><WHITESPACE><WHITESPACE>
                // where forwd means forward the request and found means <IPA> <PORT> is the required result
                // extract ip and port
                for( int m = 0; data_update_received[m] != 32; m+=n_holder )
                {
                    for( int n = 0; data_update_received[m+n] != 32; n++ )
                    {
                        switch (c)
                        {
                            case 0:
                            response[n]    = data_update_received[m+n];
                                break;
                            case 1:
                            remote_ip[n]   = data_update_received[m+n];
            //                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                                break;
                            case 2:
                            remote_port[n] = data_update_received[m+n];
            //                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                                break;
                        }
                        n_holder = n;
                    }
                    switch(c)
                    {
                    case 0:
                        response [n_holder + 1]   = 0;
                    case 1:
                        remote_ip[n_holder + 1]   = 0;
                        break;
                    case 2:
                        remote_port[n_holder + 1] = 0;
                        break;
                    }
                    c++;
                    n_holder += 2;
                }

                if( !strcmp(response, "found") )
                {
                    key_del.push_back( iter->first );
                  //  printf("\n found in fix data store\n");
                    close(local_sockfd);
                    // send remote ip remote port message telling him that i know u should have this data
                    // STORE IT

                    if((local_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
                    {
                      printf("\n-- Error : Could not create socket \n");
                      //return 1;
                    }

                    memset(data_update_message,0,sizeof(data_update_message));


                    // data sent has format str_data_entry <KEY> <H(KEY)> <DATA><WHITESPACE><WHITESPACE>
                    strcpy(data_update_message,"str_data_entry");
                    strcat(data_update_message," ");
                    strcat(data_update_message,data_store[ iter->first ].key_str);
                    strcat(data_update_message," ");
                    strcat(data_update_message,hashed_key_str);
                    strcat(data_update_message," ");
                    strcat(data_update_message,data_store[ iter->first ].data_value);
                    strcat(data_update_message,"  ");

                    // NOW, send data_update_message to the changed remote_ip and remote_port and see if it can
                    // provide me the correct node information
                    memset(&serv_addr, 0, sizeof(serv_addr));

                    serv_addr.sin_family = AF_INET;
                    int p = atoi( remote_port );
                    serv_addr.sin_port = htons(p);
                    serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

                    while(connect(local_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);
                    if( send(local_sockfd , data_update_message , sizeof(data_update_message) , 0) < 0)
                    {
                        puts("Send failed");
                    }
                }
                else
                    continue;

                close(local_sockfd);

            } while( !strcmp(response, "forwd") );
        }

    }

    for(int i = 0; i < key_del.size(); i++)
    {
        data_store.erase( key_del.at( i ) );
    }
    puts("-- Data distribution done");
    puts(" > ");
    fflush(stdout);
}

void _create()
{
   connected_to_ring = true;
   my_id = sha_hash_id(my_ip , my_port);

   puts("-- Attempting to create a new ring . . . . . .");
   printf("-- My Node ID : %d \n",my_id);

   current->my_id = my_id;
   strcpy( current->my_ip     , my_ip );
   strcpy( current->my_port   , my_port );
   current->pred_id = my_id;
   strcpy( current->pred_ip   , my_ip );
   strcpy( current->pred_port , my_port );
   current->succ_id = my_id;
   strcpy( current->succ_ip   , my_ip );
   strcpy( current->succ_port , my_port );

   // initialize my finger table by entering each entry as SELF
   // because I am the only node in the ring
   for( int i = 0; i < N; i++ )
   {
        current->ftable.index[i] = (unsigned long int)( my_id + pow(2,i) ) % M;
        current->ftable.id[i] = my_id;
        strcpy( current->ftable.ip[i]   ,   my_ip);
        strcpy( current->ftable.port[i] , my_port);
   }

   puts("-- New ring created");
}
void _join()
{
    my_id = sha_hash_id(my_ip , my_port);
    current->my_id = my_id;
    strcpy( current->my_ip   ,  my_ip );
    strcpy( current->my_port , my_port);

    scanf("%s", remote_ip);
    scanf("%s", remote_port);

    fflush(stdin);

    printf("-- My Node ID : %d \n",my_id);

    char response[6];
    do {

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
        {
          printf("\n-- Error : Could not create socket \n");
          return;
        }

        memset(client_message,0,sizeof(client_message));
        strcpy(client_message,"find_successor");
        strcat(client_message," ");
        strcat(client_message,my_ip);
        strcat(client_message," ");
        strcat(client_message,my_port);
        strcat(client_message,"  ");

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        int p = atoi(remote_port);
        serv_addr.sin_port = htons(p);
        serv_addr.sin_addr.s_addr = inet_addr(remote_ip);

        puts("-- Attempting to connect to server . . . . . .");
        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
        {
            puts("-- Unable to connect to server, try a different target node");
            connect_failed = true;
            return;
        }
        puts("-- Connection established");

        if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
        {
            puts("Send failed");
            return;
        }

        memset(client_received, 0, sizeof(client_received));
        memset(response       , 0, sizeof(response       ));
        memset(remote_ip      , 0, sizeof(remote_ip      ));
        memset(remote_port    , 0, sizeof(remote_port    ));

        n = read(sockfd, client_received, sizeof(client_received));

        // if I holds, then we got our answer, if II holds, then we will forward our query
        // I.  client_received has the format found <PIPA> <PPORT> <SIPA> <SPORT><WHITESPACE><WHITESPACE> i.e. info of pred and succ
        // II. client_received has the format forwd <RIPA> <RPORT><WHITESPACE><WHITESPACE> where to forward the query
        // store them in corresponding values
        //strcpy( response, client_received);
        for(int i=0;i<5;i++)
           response[i] = client_received[i];
        response[5] = 0;
        if( !strcmp( response, "found") )
        {
            int c = -1;
            int j_holder = 0;
            for( int i = 0; client_received[i] != 32; i+=j_holder )
            {
                for( int j = 0; client_received[i+j] != 32; j++ )
                {
                    switch (c)
                    {
                        case -1:
                            break;
                        case 0:
                        current->pred_ip[j]   = client_received[i+j];
        //                std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                            break;
                        case 1:
                        current->pred_port[j] = client_received[i+j];
        //                std::cout << c << " " << j << " " <<current->pred_port[j] << "\n";
                            break;
                        case 2:
                        current->succ_ip[j]   = client_received[i+j];
        //                std::cout << c << " " << j << " " <<current->succ_ip[j] << "\n";
                            break;
                        case 3:
                        current->succ_port[j] = client_received[i+j];
        //                std::cout << c << " " << j << " " <<current->succ_port[j] << "\n";
                            break;
                    }
                    j_holder = j;
                }
                switch(c)
                {
                case -1:
                    break;
                case 0:
                    current->pred_ip[j_holder + 1]     = 0;
                    break;
                case 1:
                    current->pred_port[j_holder + 1]   = 0;
                    break;
                case 2:
                    current->succ_ip[j_holder + 1]     = 0;
                    break;
                case 3:
                    current->succ_port[j_holder + 1]   = 0;
                    break;
                }
                c++;
                j_holder += 2;
            }

            current->pred_id = sha_hash_id(current->pred_ip, current->pred_port);
            current->succ_id = sha_hash_id(current->succ_ip, current->succ_port);
            current->my_id   = my_id;

        } else if( !strcmp(response, "forwd" ))
        {
            int c = 0;
            int j_holder = 0;

            // extract remote_ip and remote_port where the query will be forwarded in the next ITERATION
            for( int i = 0; client_received[i] != 32; i+=j_holder )
            {
                for( int j = 0; client_received[i+j] != 32; j++ )
                {
                    switch (c)
                    {
                        case 0:
                        response[j]    = client_received[i+j];
                            break;
                        case 1:
                        remote_ip[j]   = client_received[i+j];
        //                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                            break;
                        case 2:
                        remote_port[j] = client_received[i+j];
        //                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                            break;
                    }
                    j_holder = j;
                }
                switch(c)
                {
                case 0:
                    response [j_holder + 1]   = 0;
                case 1:
                    remote_ip[j_holder + 1]   = 0;
                    break;
                case 2:
                    remote_port[j_holder + 1] = 0;
                    break;
                }
                c++;
                j_holder += 2;
            }
        } else
            puts("IMPOSSIBLE");

        close(sockfd);

    } while( !strcmp( response, "forwd"));


//    puts("CAME OUT OF LOOP");
    // I am the second node that entered into the ring
    // Hence I can update my finger table easily
    if( current->pred_id == current->succ_id )
    {
        int rid = current->pred_id; // or succ_id
        int iid;
        int mid = my_id;

        for( int i = 0; i < N; i++ )
        {
            memset( current->ftable.ip[i]  , 0, sizeof( current->ftable.ip[i]   ) );
            memset( current->ftable.port[i], 0, sizeof( current->ftable.port[i] ) );

            iid = current->ftable.index[i];
            if(      mid < iid && iid < rid )
            {
                //insert rid
                strcpy( current->ftable.ip[i]  , remote_ip   );
                strcpy( current->ftable.port[i], remote_port );
                current->ftable.id[i] = rid;
            }
            else if( mid < rid && rid < iid )
            {
                //insert mid
                strcpy( current->ftable.ip[i]  , my_ip   );
                strcpy( current->ftable.port[i], my_port );
                current->ftable.id[i] = mid;
            }
            else if( rid < mid && mid < iid )
            {
                //insert rid
                strcpy( current->ftable.ip[i]  , remote_ip   );
                strcpy( current->ftable.port[i], remote_port );
                current->ftable.id[i] = rid;
            }
            else if( iid < mid && mid < rid )
            {
                //insert mid
                strcpy( current->ftable.ip[i]  , my_ip   );
                strcpy( current->ftable.port[i], my_port );
                current->ftable.id[i] = mid;
            }
            else if( iid < rid && rid < mid )
            {
                //insert rid
                strcpy( current->ftable.ip[i]  , remote_ip   );
                strcpy( current->ftable.port[i], remote_port );
                current->ftable.id[i] = rid;
            }
            else if( rid < iid && iid < mid )
            {
                //insert mid
                strcpy( current->ftable.ip[i]  , my_ip   );
                strcpy( current->ftable.port[i], my_port );
                current->ftable.id[i] = mid;
            }
        }
    }

    if( n < -1)
    {
        printf("\n Read Error \n");
    }

}

void _notify_successor()
{
    memset(&serv_addr, 0, sizeof(serv_addr));

    // tell current->successor to change its predecessor
    serv_addr.sin_family = AF_INET;
    int p = atoi( current->succ_port );
    serv_addr.sin_port = htons(p);
    serv_addr.sin_addr.s_addr = inet_addr( current->succ_ip );

    memset(client_message,0,sizeof(client_message));
    while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);
       // printf("Error : Connection Failed");

    strcpy(client_message,"updpredecessor");
    strcat(client_message," ");
    strcat(client_message,current->my_ip);
    strcat(client_message," ");
    strcat(client_message,current->my_port);
    strcat(client_message,"  ");

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

}

void _put_data()
{
    char response[6];

    scanf("%s", c_key_str);
    // takes in the <SPACE> for proper functioning of cin.getline()
    fgetc( stdin );
    std::cin.getline( c_data_value, 200 );

    char temp[1] = "";

    c_hashed_key = sha_hash_id(c_key_str, temp);

    sprintf( c_hashed_key_str, "%d", c_hashed_key );

    // check initially my own finger table
    strcpy( remote_port , my_port );
    strcpy( remote_ip   , my_ip   );

    // client_message will change only when found is received
    memset(client_message,0,sizeof(client_message));

    strcpy(client_message,"return_f_entry");
    strcat(client_message," ");
    strcat(client_message,my_ip);
    strcat(client_message," ");
    strcat(client_message,my_port);
    strcat(client_message," ");
    strcat(client_message,c_hashed_key_str);
    strcat(client_message,"  ");

    do
    {
        // I will send a message to server in EACH ITERATION
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
        {
          printf("\n-- Error : Could not create socket \n");
          //return 1;
        }

        // NOW, send client_message to the changed remote_ip and remote_port and see if it can
        // provide me the correct node information
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        int p = atoi( remote_port );
        serv_addr.sin_port = htons(p);
        serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

        while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);
           // printf("Error : Connection Failed");
//            puts("SENDING");
//            puts(client_message);
        if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
        {
            puts("Send failed");
        }

        memset(client_received, 0, sizeof(client_received));

        /*
        while((n = read(sockfd, client_received, sizeof(client_received))) > 0)
        {
            if(fputs(client_received, stdout) == EOF)
            {
                printf("\n Error : Fputs error");
            }
            printf("\n");
        }
        */

        n = read(sockfd, client_received, sizeof(client_received));

        int c = 0;
        int j_holder = 0;
        memset( response   , 0, sizeof(response   ));
        memset( remote_ip  , 0, sizeof(remote_ip  ));
        memset( remote_port, 0, sizeof(remote_port));

        // client_received has the format either forwd/found <IPA> <PORT><WHITESPACE><WHITESPACE>
        // where forwd means forward the request and found means <IPA> <PORT> is the required result
        // extract ip and port
        for( int i = 0; client_received[i] != 32; i+=j_holder )
        {
            for( int j = 0; client_received[i+j] != 32; j++ )
            {
                switch (c)
                {
                    case 0:
                    response[j]    = client_received[i+j];
                        break;
                    case 1:
                    remote_ip[j]   = client_received[i+j];
    //                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                        break;
                    case 2:
                    remote_port[j] = client_received[i+j];
    //                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                        break;
                }
                j_holder = j;
            }
            switch(c)
            {
            case 0:
                response [j_holder + 1]   = 0;
            case 1:
                remote_ip[j_holder + 1]   = 0;
                break;
            case 2:
                remote_port[j_holder + 1] = 0;
                break;
            }
            c++;
            j_holder += 2;
        }

        if( !strcmp( response, "found" ))
        {
            close(sockfd);
            // send remote ip remote port message telling him that i know u should have this data
            // STORE IT

            if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
            {
              printf("\n-- Error : Could not create socket \n");
              //return 1;
            }

            memset(client_message,0,sizeof(client_message));

            // data sent has format str_data_entry <KEY> <H(KEY)> <DATA><WHITESPACE><WHITESPACE>
            strcpy(client_message,"str_data_entry");
            strcat(client_message," ");
            strcat(client_message,c_key_str);
            strcat(client_message," ");
            strcat(client_message,c_hashed_key_str);
            strcat(client_message," ");
            strcat(client_message,c_data_value);
            strcat(client_message,"  ");

            // NOW, send client_message to the changed remote_ip and remote_port and see if it can
            // provide me the correct node information
            memset(&serv_addr, 0, sizeof(serv_addr));

            serv_addr.sin_family = AF_INET;
            int p = atoi( remote_port );
            serv_addr.sin_port = htons(p);
            serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

            while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);
               // printf("Error : Connection Failed");
    //            puts("SENDING");
    //            puts(client_message);
            if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
            {
                puts("Send failed");
            }
        }
        else if( !strcmp( response, "forwd" ))
        {
            //puts("client received forwd from server");
        }
        else
            puts("IMPOSSIBLE");

        close(sockfd);
    }while( !strcmp(response , "forwd"));
}

void _get_data()
{
    char response[6];

    scanf("%s", c_key_str);

    char temp[1] = "";

    c_hashed_key = sha_hash_id(c_key_str, temp);

    sprintf( c_hashed_key_str, "%d", c_hashed_key );

    // check initially my own finger table
    strcpy( remote_port , my_port );
    strcpy( remote_ip   , my_ip   );

    // client_message will change only when found is received
    memset(client_message,0,sizeof(client_message));

    strcpy(client_message,"return_f_entry");
    strcat(client_message," ");
    strcat(client_message,my_ip);
    strcat(client_message," ");
    strcat(client_message,my_port);
    strcat(client_message," ");
    strcat(client_message,c_hashed_key_str);
    strcat(client_message,"  ");

    do
    {
        // I will send a message to server in EACH ITERATION
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
        {
          printf("\n-- Error : Could not create socket \n");
          //return 1;
        }

        // NOW, send client_message to the changed remote_ip and remote_port and see if it can
        // provide me the correct node information
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        int p = atoi( remote_port );
        serv_addr.sin_port = htons(p);
        serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

        while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);

        if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
        {
            puts("Send failed");
        }

        memset(client_received, 0, sizeof(client_received));

        n = read(sockfd, client_received, sizeof(client_received));

        int c = 0;
        int j_holder = 0;
        memset( response   , 0, sizeof(response   ));
        memset( remote_ip  , 0, sizeof(remote_ip  ));
        memset( remote_port, 0, sizeof(remote_port));

        // client_received has the format either forwd/found <IPA> <PORT><WHITESPACE><WHITESPACE>
        // where forwd means forward the request and found means <IPA> <PORT> is the required result
        // extract ip and port
        for( int i = 0; client_received[i] != 32; i+=j_holder )
        {
            for( int j = 0; client_received[i+j] != 32; j++ )
            {
                switch (c)
                {
                    case 0:
                    response[j]    = client_received[i+j];
                        break;
                    case 1:
                    remote_ip[j]   = client_received[i+j];
                        break;
                    case 2:
                    remote_port[j] = client_received[i+j];
                        break;
                }
                j_holder = j;
            }
            switch(c)
            {
            case 0:
                response [j_holder + 1]   = 0;
            case 1:
                remote_ip[j_holder + 1]   = 0;
                break;
            case 2:
                remote_port[j_holder + 1] = 0;
                break;
            }
            c++;
            j_holder += 2;
        }

        if( !strcmp( response, "found" ))
        {
            close(sockfd);
            // send remote ip remote port message telling him that i know u should have this data
            // STORE IT

            if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
            {
              printf("\n-- Error : Could not create socket \n");
              //return 1;
            }

            memset(client_message,0,sizeof(client_message));

            // data sent has format get_data_entry <KEY> <WHITESPACE><WHITESPACE>
            strcpy(client_message,"get_data_entry");   //////////////////////////////////////////////
            strcat(client_message," ");
            strcat(client_message,c_key_str);
            strcat(client_message,"  ");

            // NOW, send client_message to the changed remote_ip and remote_port and see if it can
            // provide me the correct node information
            memset(&serv_addr, 0, sizeof(serv_addr));

            serv_addr.sin_family = AF_INET;
            int p = atoi( remote_port );
            serv_addr.sin_port = htons(p);
            serv_addr.sin_addr.s_addr = inet_addr( remote_ip );

            while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);

            if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
            {
                puts("Send failed");
            }


            memset(client_received, 0, sizeof(client_received));

            n = read(sockfd, client_received, sizeof(client_received));

            //memset( response      , 0 , sizeof( response     ));
            memset( c_data_value  , 0 , sizeof( c_data_value ));

            // client_received has the format <KEY> <H(KEY)> <DATA><WHITESPACE><WHITESPACE>

            strcpy( c_data_value , client_received );
            if(!strcmp(c_data_value,""))
                strcpy(c_data_value,"Data Not Found !!!");
            printf("\n Key : %s       Data Value : %s \n", c_key_str , c_data_value);
        }
        else if( !strcmp( response, "forwd" ))
        {
            // forward the query
        }
        else
            puts("IMPOSSIBLE");

        close(sockfd);
    }while( !strcmp(response , "forwd"));

}

void _dump_addr()
{
    memset(client_message,0,sizeof(client_message));

    strcpy(client_message,"give_your_dump");
    strcat(client_message," ");
    strcat(client_message,my_ip);
    strcat(client_message," ");
    strcat(client_message,my_port);
    strcat(client_message,"  ");

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      printf("\n-- Error : Could not create socket \n");
      //return 1;
    }

    // NOW, send client_message to the changed c_remote_ip and c_remote_port and see if it can
    // provide me the correct node information
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    int p = atoi( c_remote_port );
    serv_addr.sin_port = htons(p);
    serv_addr.sin_addr.s_addr = inet_addr( c_remote_ip );

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        puts("--No such node in the ring");
        return;
    }

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received, 0, sizeof(client_received));

    ///////////////////////////////////////////////////////////////

    n = read(sockfd, client_received, sizeof(client_received));

    //puts("client received");
    //puts(client_received);

    while(strcmp(client_received , "over") && strcmp(client_received , "no_data"))
    {
        memset( c_key_str, 0 , sizeof( c_key_str  ));
        memset( c_data_value, 0, sizeof(c_data_value));

        int c = 1;
        int j_holder = 0;

        // client_received has the format <KEY> <DATA><WHITESPACE><WHITESPACE>
        for( int i = 0; client_received[i] != 32; i+=j_holder )
        {
            for( int j = 0; client_received[i+j] != 32 || c == 2; j++ )
            {
                if( c==2 && client_received[i+j] == 32 && client_received[i+j+1] == 32 )
                    break;
                switch (c)
                {
                    case 1:
                    c_key_str[j]          = client_received[i+j];
 //                   std::cout << c << " " << j << " " <<c_key_str[j] << "\n";
                        break;

                    case 2:
                    c_data_value[j]       =  client_received[i+j];
 //                   std::cout << c << " " << j << " " <<c_data_value[j] << "\n";
                        break;
                }
                j_holder = j;
            }
            switch(c)
            {

            case 1:
                c_key_str[j_holder + 1]   = 0;
                break;

            case 2:
                c_data_value[j_holder + 1]     = 0;
                break;
            }
            c++;
            j_holder += 2;
        }

        printf("\nKey : %5d    |          Data Value : %s \n", atoi(c_key_str) , c_data_value);

        memset(client_message , 0 , sizeof(client_message));
        strcpy(client_message , "give_data");

        if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
        {
            puts("Send failed");
        }

        memset(client_received , 0 , sizeof(client_received));
        n = read(sockfd, client_received, sizeof(client_received));

    }

    if( !strcmp(client_received , "no_data") )
            puts("-- No Data Found !!!");

    //take successor ip
    memset(client_message , 0 , sizeof(client_message));
    strcpy(client_message , "give_ip");

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received , 0 , sizeof(client_received));
    n = read(sockfd, client_received, sizeof(client_received));

    memset(c_remote_ip , 0 , sizeof(c_remote_ip));
    strcpy(c_remote_ip , client_received);

    //take successor port
    memset(client_message , 0 , sizeof(client_message));
    strcpy(client_message , "give_port");

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received , 0 , sizeof(client_received));
    n = read(sockfd, client_received, sizeof(client_received));

    memset(c_remote_port , 0 , sizeof(c_remote_port));
    strcpy(c_remote_port , client_received);

    close(sockfd);
}

void _dump_all()
{

    // First, print my own node's contents

    printf("\n-- Information at this Node(Node ID - %d ) \n\n", my_id);

    typedef std::map<int, struct data_element>::const_iterator MapIterator;
    for (MapIterator iter = data_store.begin(); iter != data_store.end(); iter++)
    {
        printf("\nKey : %5d    |          Data Value : %s \n", iter->first, data_store[iter->first].data_value);
    }

    printf("\n----------------------------------------------------------\n\n");

    memset(c_remote_ip , 0 , sizeof(c_remote_ip) );
    memset(c_remote_port , 0 , sizeof(c_remote_port) );

    strcpy(c_remote_ip   , current->succ_ip);
    strcpy(c_remote_port , current->succ_port);

    int c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);


    while(c_remote_id != current->my_id)
    {
        printf("\n-- Information at Node %d \n\n", c_remote_id);
        _dump_addr();
        c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);
        printf("\n----------------------------------------------------------\n\n");
    }
}


void _finger()
{
    printf("\n-- Nodes present in the ring : \n\n");

    printf("|--------------------------------------------------|\n");
    printf("|    Node ID  |      Node IP      |    Node Port   |\n");
    printf("|--------------------------------------------------|\n");

    printf("|    %5d    |%15s    |    %6s      |\n", my_id , current->my_ip , current->my_port);

    memset(c_remote_ip , 0 , sizeof(c_remote_ip) );
    memset(c_remote_port , 0 , sizeof(c_remote_port) );

    strcpy(c_remote_ip   , current->succ_ip);
    strcpy(c_remote_port , current->succ_port);

    int c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);


    while(c_remote_id != current->my_id)
    {

        printf("|    %5d    |%15s    |    %6s   |\n", c_remote_id , c_remote_ip , c_remote_port);

        memset(client_message,0,sizeof(client_message));

        strcpy(client_message,"give_your_info");
        strcat(client_message," ");
        strcat(client_message,my_ip);
        strcat(client_message," ");
        strcat(client_message,my_port);
        strcat(client_message,"  ");

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
        {
          printf("\n-- Error : Could not create socket \n");
          //return 1;
        }

        // NOW, send client_message to the changed c_remote_ip and c_remote_port and see if it can
        // provide me the correct node information
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        int p = atoi( c_remote_port );
        serv_addr.sin_port = htons(p);
        serv_addr.sin_addr.s_addr = inet_addr( c_remote_ip );

        while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);

        if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
        {
            puts("Send failed");
        }


        // client receiving node info
        memset(client_received, 0, sizeof(client_received));
        n = read(sockfd, client_received, sizeof(client_received));

        int c = 1;
        int j_holder = 0;

        // extract remote_ip and remote_port where the query will be forwarded in the next ITERATION
        for( int i = 0; client_received[i] != 32; i+=j_holder )
        {
            for( int j = 0; client_received[i+j] != 32; j++ )
            {
                switch (c)
                {
                    case 1:
                    c_remote_ip[j]   = client_received[i+j];
    //                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                        break;
                    case 2:
                    c_remote_port[j] = client_received[i+j];
    //                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                        break;
                }
                j_holder = j;
            }
            switch(c)
            {
            case 1:
                c_remote_ip[j_holder + 1]   = 0;
                break;
            case 2:
                c_remote_port[j_holder + 1] = 0;
                break;
            }
            c++;
            j_holder += 2;
        }

        c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);
        close(sockfd);

    }
    printf("|--------------------------------------------------|\n\n");

}


void _finger_addr()
{
    int count = 0 , c_id ;
    memset(client_message,0,sizeof(client_message));

    strcpy(client_message,"gve_finger_tbl");
    strcat(client_message," ");
    strcat(client_message,my_ip);
    strcat(client_message," ");
    strcat(client_message,my_port);
    strcat(client_message,"  ");

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      printf("\n-- Error : Could not create socket \n");
      //return 1;
    }

    // NOW, send client_message to the changed c_remote_ip and c_remote_port and see if it can
    // provide me the correct node information
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    int p = atoi( c_remote_port );
    serv_addr.sin_port = htons(p);
    serv_addr.sin_addr.s_addr = inet_addr( c_remote_ip );

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        puts("-- No such node in the ring");
        return;
    }

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received, 0, sizeof(client_received));

    puts("    |------------------------------------------------------------------|");
    puts("    |    Index    |   Node ID  |  Node IP Address   | Node port number |");
    puts("    |-------------|------------|--------------------|------------------|");

    n = read(sockfd, client_received, sizeof(client_received));

    //puts("client received");
    //puts(client_received);

    while( count++ < N )
    {
        memset( c_index, 0 , sizeof( c_index  ));
        memset( c_ip_entry, 0, sizeof(c_ip_entry));
        memset( c_port_entry, 0, sizeof(c_port_entry));

        int c = 1;
        int j_holder = 0;

        // client_received has the format <KEY> <DATA><WHITESPACE><WHITESPACE>
        for( int i = 0; client_received[i] != 32; i+=j_holder )
        {
            for( int j = 0; client_received[i+j] != 32 ; j++ )
            {
                switch (c)
                {
                    case 1:
                    c_index[j]          = client_received[i+j];
 //                   std::cout << c << " " << j << " " <<c_key_str[j] << "\n";
                        break;

                    case 2:
                    c_ip_entry[j]       =  client_received[i+j];
 //                   std::cout << c << " " << j << " " <<c_data_value[j] << "\n";
                        break;

                    case 3:
                    c_port_entry[j]       =  client_received[i+j];
    //                   std::cout << c << " " << j << " " <<c_data_value[j] << "\n";
                        break;
                }
                j_holder = j;
            }
            switch(c)
            {

            case 1:
                c_index[j_holder + 1]          = 0;
                break;

            case 2:
                c_ip_entry[j_holder + 1]       = 0;
                break;

            case 3:
                c_port_entry[j_holder + 1]     = 0;
                break;
            }
            c++;
            j_holder += 2;
        }

        c_id = sha_hash_id(c_ip_entry , c_port_entry);

        printf("    |%7s      |%7lu     |%15s     |  %10s      |\n", c_index , c_id , c_ip_entry , c_port_entry );

        if( count != N )
        {
            memset(client_message , 0 , sizeof(client_message));
            strcpy(client_message , "give_finger_table_entry");

            if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
            {
                puts("Send failed");
            }

            memset(client_received , 0 , sizeof(client_received));
            n = read(sockfd, client_received, sizeof(client_received));

        }

    }

     puts("    |------------------------------------------------------------------|");

    //take successor ip
    memset(client_message , 0 , sizeof(client_message));
    strcpy(client_message , "give_ip");




    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received , 0 , sizeof(client_received));
    n = read(sockfd, client_received, sizeof(client_received));

    memset(c_remote_ip , 0 , sizeof(c_remote_ip));
    strcpy(c_remote_ip , client_received);

    //take successor port
    memset(client_message , 0 , sizeof(client_message));
    strcpy(client_message , "give_port");

    if( send(sockfd , client_message , sizeof(client_message) , 0) < 0)
    {
        puts("Send failed");
    }

    memset(client_received , 0 , sizeof(client_received));
    n = read(sockfd, client_received, sizeof(client_received));

    memset(c_remote_port , 0 , sizeof(c_remote_port));
    strcpy(c_remote_port , client_received);

    close(sockfd);

}


void _finger_all()
{
    // First, print my own node's contents

    printf("\n-- Finger Table of this Node (Node ID : %d) \n\n",my_id);

    puts("    |------------------------------------------------------------------|");
    puts("    |    Index    |   Node ID  |  Node IP Address   | Node port number |");
    puts("    |-------------|------------|--------------------|------------------|");
    for(int i = 0; i < N; i++)
    {
        printf("    |%7lu      |%7lu     |%15s     |  %10s      |\n", current->ftable.index[i], current->ftable.id[i], current->ftable.ip[i], current->ftable.port[i] );
    }
    puts("    |------------------------------------------------------------------|");

    memset(c_remote_ip , 0 , sizeof(c_remote_ip) );
    memset(c_remote_port , 0 , sizeof(c_remote_port) );

    strcpy(c_remote_ip   , current->succ_ip);
    strcpy(c_remote_port , current->succ_port);

    int c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);


    while(c_remote_id != current->my_id)
    {
        printf("\n Finger Table of Node with Node ID  %d \n\n", c_remote_id);
        _finger_addr();
        c_remote_id = sha_hash_id(c_remote_ip , c_remote_port);
    }


}
