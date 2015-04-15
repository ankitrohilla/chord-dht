/**********************  MAKE_STRING_4_RESPONSE      ***********************/
/**********************  MAKE_STRING_2_RESPONSE      ***********************/
/**********************  FIND_SUCCESSOR              ***********************/
/**********************  UPDATE_PREDECESSOR         ***********************/
/**********************  CHECK_TABLE                 ***********************/
/**********************  STORE_DATA_ENTRY            ***********************/
/**********************  GET_DATA_ENTRY              ***********************/
/**********************  GIVE_DUMP                   ***********************/
/**********************  GIVE_FINGER_TABLE           ***********************/


int sha_hash_id(char *ip,char *port);

// variables to handle data
int  s_key;
char s_key_str[ 5 ];

char s_data_value[ 200 ];

int  s_hashed_key;
char s_hashed_key_str[ 5 ];

char repl[60], response[6];

void make_string_4_response( char* pipa, char* pport, char* sipa, char* sport)
{
    memset( repl, 0, sizeof(repl) );

    strcpy(repl, response);
    strcat(repl, " ");
    strcat(repl, pipa );
    strcat(repl, " ");
    strcat(repl, pport);
    strcat(repl, " ");
    strcat(repl, sipa);
    strcat(repl, " ");
    strcat(repl, sport);
    strcat(repl, "  ");
//    printf("\nrepl in check table : %s\n",repl);

//    puts(repl);
}

void make_string_2_response( char* ipa, char* port )
{
    memset( repl, 0, sizeof(repl) );

    strcpy(repl, response);
    strcat(repl, " ");
    strcat(repl, ipa );
    strcat(repl, " ");
    strcat(repl, port);
    strcat(repl, "  ");

 //   printf("\nrepl in check table : %s\n",repl);
//    puts(repl);
}

// may have to check finger table if RID is not my immediate successor
char* _find_successor()
{
    int c = 0;
    int j_holder = 0;

    // server_received has the format find_successor <IPA> <PORT><WHITESPACE><WHITESPACE>
    // extract remote_ip and remote_port
    for( int i = 0; server_received[i] != 32; i+=j_holder )
    {                                                         // abc bhr jhsdf
        for( int j = 0; server_received[i+j] != 32; j++ )
        {
            switch (c)
            {
                case 0:
//                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                    break;
                case 1:
                remote_ip[j] = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                    break;
                case 2:
                remote_port[j]   = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_port[j] << "\n";
                    break;
            }
            j_holder = j;
        }
        switch(c)
        {
        case 0:
            break;
        case 1:
            remote_ip[j_holder + 1]   = 0;
            break;
        case 2:
            remote_port[j_holder + 1]     = 0;
            break;
        }
        c++;
        j_holder += 2;
    }

    // Now I have remote_ip and remote_port

//    std::cout << "PRED ID stored - " << current->pred_id;
//    std::cout << "SUCC ID stored - " << current->succ_id;
//    std::cout << "MY   ID stored - " << current->my_id;

    // the repl will be divided as <PIPA> <PPORT> <SIPA> <SPORT><WHITESPACE><WHITESPACE>
    memset(repl, 0, sizeof(repl));

    int rid = sha_hash_id(remote_ip, remote_port);
    int mid = current->my_id;
    int sid = current->succ_id;

//    puts("REMOTE ID");

    // special case when the ring has only ME
    if( sid == mid )
    {
        strcpy(response, "found");
//        puts( "THIS IS EXPECTED TO PRINT for the first time only");
        
        memset( current->succ_ip , 0 , sizeof(current->succ_ip) );
        memset( current->succ_port , 0 , sizeof(current->succ_port) );

        memset( current->pred_ip , 0 , sizeof(current->pred_ip) );
        memset( current->pred_port , 0 , sizeof(current->pred_port) );

        current->succ_id = rid;
        strcpy( current->succ_ip   ,   remote_ip );
        strcpy( current->succ_port , remote_port );

        current->pred_id = rid;
        strcpy( current->pred_ip   ,   remote_ip );
        strcpy( current->pred_port , remote_port );

        make_string_4_response(my_ip, my_port, my_ip, my_port);

        int iid;
        // now that I know a new node came, I will update my finger table
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
                //inser rid
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

        return repl;
    }

    // CASES to check the relative positions of my_id, su_id and remote_id
    if( sid < mid && mid <= rid )
    {
        strcpy(response, "found");
        make_string_4_response(my_ip, my_port, current->succ_ip, current->succ_port);
        
        memset( current->succ_ip , 0 , sizeof(current->succ_ip) );
        memset( current->succ_port , 0 , sizeof(current->succ_port) );

        current->succ_id   = rid;
        strcpy( current->succ_ip   , remote_ip  );
        strcpy( current->succ_port , remote_port);

        return repl;
    } else if( rid < sid && sid < mid )
    {
        strcpy(response, "found");
        make_string_4_response(my_ip, my_port, current->succ_ip, current->succ_port);

        memset( current->succ_ip , 0 , sizeof(current->succ_ip) );
        memset( current->succ_port , 0 , sizeof(current->succ_port) );

        current->succ_id   = rid;
        strcpy( current->succ_ip   , remote_ip  );
        strcpy( current->succ_port , remote_port);

        return repl;
    } else if( mid <= rid && rid < sid )
    {
        strcpy(response, "found");
        make_string_4_response(my_ip, my_port, current->succ_ip, current->succ_port);

        memset( current->succ_ip , 0 , sizeof(current->succ_ip) );
        memset( current->succ_port , 0 , sizeof(current->succ_port) );

        current->succ_id   = rid;
        strcpy( current->succ_ip   , remote_ip  );
        strcpy( current->succ_port , remote_port);

        return repl;
    } else if( rid < mid )
    { // FINGER TABLE CASE

        strcpy(response, "forwd");

        int i = 0;

        int diff = rid + M - mid;

        // exit the loop when 2^i >= diff
        // i--; after the loop ensures correctness
        while( current->ftable.id[i] <rid || current->ftable.id[i] > mid && i<N )
        {
            i++;
        }
        i--;

        // MODIFY WHAT TO SEND TOMORROW

        make_string_2_response(current->ftable.ip[i] , current->ftable.port[i]);
//        puts("rid<mid");
//        std::cout<<"i = "<<i<<" diff = "<<diff<<"\n";
//        puts(repl);
        return repl;

    } else if( mid < rid )
    { // FINGER TABLE CASE

        strcpy(response, "forwd");

        int i = 0;

        int diff = rid - mid;

        // exit the loop when 2^i >= diff
        // i--; after the loop ensures correctness
        while( current->ftable.id[i] < rid && current->ftable.id[i] > mid && i<N )
        {
            i++;
        }
        i--;

        // MODIFY WHAT TO SEND TOMORROW

        // if, by luck, best case
        if( pow(2,i) == diff )
        {
            strcpy(response, "found");
        }

        make_string_2_response(current->ftable.ip[i] , current->ftable.port[i]);

        return repl;
    }
}

void _update_predecessor()
{

    int c = 0;
    int j_holder = 0;

    memset( current->pred_ip , 0 , sizeof(current->pred_ip) );
    memset( current->pred_port , 0 , sizeof(current->pred_port) );

    // server_received has the format updpredecessor <IPA> <PORT><WHITESPACE><WHITESPACE>
    // extract remote_ip and remote_port
    for( int i = 0; server_received[i] != 32; i+=j_holder )
    {                                                         // abc bhr jhsdf
        for( int j = 0; server_received[i+j] != 32; j++ )
        {
            switch (c)
            {
                case 0:
//                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                    break;
                case 1:
                current -> pred_ip[j]     = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                    break;
                case 2:
                current -> pred_port[j]   = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_port[j] << "\n";
                    break;
            }
            j_holder = j;
        }
        switch(c)
        {
        case 0:
            break;
        case 1:
            remote_ip[j_holder + 1]   = 0;
            break;
        case 2:
            remote_port[j_holder + 1]     = 0;
            break;
        }
        c++;
        j_holder += 2;
    }

    current->pred_id = sha_hash_id(current->pred_ip, current->pred_port);

    printf("-- Predecessor updated to - %s:%s\n", current->pred_ip, current->pred_port);
    fix_data_store = true;
}

// checks finger table and returns the node information of the appropriate node
char* _check_table()
{
    int c = 0;
    int j_holder = 0;
    char iid_str[7];

    memset( remote_ip , 0 , sizeof(remote_ip) );
    memset( remote_port , 0 , sizeof(remote_port) );
    memset( iid_str , 0 , sizeof(iid_str) );

    int iid;

    // server_received has the format return_f_entry <IPA> <PORT> <IID><WHITESPACE><WHITESPACE>
    // extract remote_ip, remote_port and iid_str
    for( int i = 0; server_received[i] != 32; i+=j_holder )
    {
        for( int j = 0; server_received[i+j] != 32; j++ )
        {
            switch (c)
            {
                case 0:
//                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                    break;
                case 1:
                remote_ip[j]     = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                    break;
                case 2:
                remote_port[j]   = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_port[j] << "\n";
                    break;
                case 3:
                iid_str[j]       =  server_received[i+j];
                    break;
            }
            j_holder = j;
        }
        switch(c)
        {
        case 0:
            break;
        case 1:
            remote_ip[j_holder + 1]   = 0;
            break;
        case 2:
            remote_port[j_holder + 1] = 0;
            break;
        case 3:
            iid_str[j_holder + 1]     = 0;
            break;
        }
        c++;
        j_holder += 2;
    }

    iid = atoi( iid_str );

    int mid = my_id;
    int sid = current->succ_id;
    int diff;

    memset(response, 0, sizeof(response));

    // first 3 cases are trivial, rest will forward the query
    if(mid == iid)
    {
        strcpy(response , "found");
        make_string_2_response( current->my_ip , current->my_port);
        return repl;

    }
    if( mid < iid && iid  <= sid )
    {
        strcpy(response, "found");
        make_string_2_response( current->succ_ip, current->succ_port );
        return repl;
    }
    else if( sid <= mid && mid < iid )
    {
        strcpy(response, "found");
        make_string_2_response( current->succ_ip, current->succ_port );
        return repl;
    }
    else if( iid <= sid && sid <= mid )
    {
        strcpy(response, "found");
        make_string_2_response( current->succ_ip, current->succ_port );
        return repl;
    }
    else if( mid < iid )
    {
        strcpy(response, "forwd");

        int i = 0;

        int diff = iid - mid;

        // exit the loop when 2^i >= diff

        while( pow(2,i)<diff )
        {
            i++;
        }
        i--;

        int closest_pred_sid = sha_hash_id(current->ftable.ip[i] , current->ftable.port[i]);

//        printf(" %d %d %d", closest_pred_sid, iid, mid);

        // smaller than mid OR greater than iid
        if( closest_pred_sid >= iid || closest_pred_sid <= mid )
            strcpy(response , "found");
        make_string_2_response(current->ftable.ip[i] , current->ftable.port[i]);

        return repl;
    }
    else if( iid < mid )
    {

        strcpy(response, "forwd");

        int i = 0;

        // M to accomodate rounding off
        int diff = iid + M - mid;

        // exit the loop when 2^i >= diff
        // i--; after the loop ensures correctness
        while( pow(2,i)<diff )
        {
            i++;
        }
        i--;

        int closest_pred_sid = sha_hash_id(current->ftable.ip[i] , current->ftable.port[i]);

   //     printf(" %d %d %d", closest_pred_sid, iid, mid); /////

        // smaller than mid AND greater than iid
        if( closest_pred_sid >= iid && closest_pred_sid <= mid )
            strcpy(response , "found");
        make_string_2_response(current->ftable.ip[i] , current->ftable.port[i]);

        return repl;
    }
}

void _store_data_entry()
{
    int c = 0;
    int j_holder = 0;

    memset( s_key_str , 0 , sizeof(s_key_str) );
    memset( s_hashed_key_str , 0 , sizeof(s_hashed_key_str) );
    memset( s_data_value , 0 , sizeof(s_data_value) );

    // server_received has the format str_data_entry <KEY> <H(KEY)> <DATA><WHITESPACE><WHITESPACE>
    for( int i = 0; server_received[i] != 32; i+=j_holder )
    {
        for( int j = 0; server_received[i+j] != 32 || c == 3; j++ )
        {
            if( c==3 && server_received[i+j] == 32 && server_received[i+j+1] == 32 )
                break;
            switch (c)
            {
                case 0:
//                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                    break;
                case 1:
                s_key_str[j]          = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                    break;
                case 2:
                s_hashed_key_str[j]   = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_port[j] << "\n";
                    break;
                case 3:
                s_data_value[j]       =  server_received[i+j];

                    break;
            }
            j_holder = j;
        }
        switch(c)
        {
        case 0:
            break;
        case 1:
            s_key_str[j_holder + 1]   = 0;
            break;
        case 2:
            s_hashed_key_str[j_holder + 1] = 0;
            break;
        case 3:
            s_data_value[j_holder + 1]     = 0;
            break;
        }
        c++;
        j_holder += 2;
    }

    s_key        = atoi(        s_key_str );
    s_hashed_key = atoi( s_hashed_key_str );

    printf("\n-- Command received - '%s'  '%s'\n-- Storing . . . . . .\n", s_key_str , s_data_value);
    fflush(stdout);

    struct data_element new_entry;
    //memset(new_entry, 0, sizeof(new_entry));

    strcpy( new_entry.key_str       , s_key_str   );
    strcpy( new_entry.data_value    , s_data_value);
    new_entry.hashed_key = s_hashed_key;

    data_store[s_key] = new_entry;

}



char* _get_data_entry()
{
    int c = 0;
    int j_holder = 0;

    memset( s_key_str , 0 , sizeof(s_key_str) );
    //memset( s_hashed_key_str , 0 , sizeof(s_hashed_key_str) );
    memset( s_data_value , 0 , sizeof(s_data_value) );

    // server_received has the format get_data_entry <KEY><WHITESPACE><WHITESPACE>
    for( int i = 0; server_received[i] != 32; i+=j_holder )
    {
        for( int j = 0; server_received[i+j] != 32 || c == 3; j++ )
        {
            if( c==3 && server_received[i+j] == 32 && server_received[i+j+1] == 32 )
                break;
            switch (c)
            {
                case 0:
//                      std::cout << c << " " << j << " " <<current->pred_ip[j] << "\n";
                    break;
                case 1:
                s_key_str[j]          = server_received[i+j];
//                      std::cout << c << " " << j << " " <<remote_ip[j] << "\n";
                    break;
            }
            j_holder = j;
        }
        switch(c)
        {
        case 0:
            break;
        case 1:
            s_key_str[j_holder + 1]   = 0;
            break;
        }
        c++;
        j_holder += 2;
    }

    s_key        = atoi(        s_key_str );

    memset( repl , 0 , sizeof( repl ));

    strcpy( repl , data_store[s_key].data_value );
    if( !strcmp(repl, "") )
        data_store.erase(s_key);



    return repl;

}

void _give_dump()
{
    typedef std::map<int, struct data_element>::const_iterator MapIterator;

    for (MapIterator iter = data_store.begin(); iter != data_store.end(); iter++)
    {
        memset(server_message,0,sizeof(server_message));

        strcpy(server_message , data_store[iter->first].key_str);
        strcat(server_message ," ");
        strcat(server_message , data_store[iter->first].data_value);
        strcat(server_message , "  ");

//        puts("Server_message");
//        puts(server_message);
        write(connfd, server_message, strlen(server_message));

        memset(server_received, 0, sizeof(server_received));
        while(n=recv(connfd, server_received, sizeof(server_received), 0)<0);
    }

    memset(server_message,0,sizeof(server_message));

    if( data_store.size() != 0 )
        strcpy(server_message , "over");
    else
        strcpy(server_message , "no_data");

    write(connfd, server_message, strlen(server_message));

    //for sending successor ip

    memset(server_received, 0, sizeof(server_received));
    n=recv(connfd, server_received, sizeof(server_received), 0);

    memset(server_message,0,sizeof(server_message));
    strcpy(server_message , current->succ_ip);

    write(connfd, server_message, strlen(server_message));
//    puts(server_message);

    //for sending successor port

    memset(server_received, 0, sizeof(server_received));
    n=recv(connfd, server_received, sizeof(server_received), 0);

    memset(server_message,0,sizeof(server_message));
    strcpy(server_message , current->succ_port);

    write(connfd, server_message, sizeof(server_message));
//    puts(server_message);

}


void _give_finger_table()
{
    char index[ 5 ];

    for (int i = 0 ; i < N ; i++)
    {
        memset(server_message,0,sizeof(server_message));

        sprintf(index , "%d" , current->ftable.index[i] );

        strcpy(server_message , index);
        strcat(server_message ," ");
        strcat(server_message , current->ftable.ip[i]);
        strcat(server_message ," ");
        strcat(server_message , current->ftable.port[i]);
        strcat(server_message , "  ");

//        puts("Server_message");
//        puts(server_message);
        write(connfd, server_message, strlen(server_message));

        memset(server_received, 0, sizeof(server_received));
        while(n=recv(connfd, server_received, sizeof(server_received), 0)<0);
    }


    //for sending successor ip

    memset(server_message,0,sizeof(server_message));
    strcpy(server_message , current->succ_ip);

    write(connfd, server_message, strlen(server_message));
//    puts(server_message);

    //for sending successor port

    memset(server_received, 0, sizeof(server_received));
    n=recv(connfd, server_received, sizeof(server_received), 0);

    memset(server_message,0,sizeof(server_message));
    strcpy(server_message , current->succ_port);

    write(connfd, server_message, sizeof(server_message));
//    puts(server_message);

}
