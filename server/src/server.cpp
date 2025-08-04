#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>

#include <cstring>
#include <chrono>

#include "server.hpp"
#include "../../shared/log.hpp"
#include "../../shared/util.hpp"


bool Server::start(const ServerConfig& config) {
    m_config = config;

    printf("Starting server...\n");
    if(sam3CreateSilentSession(&this->session,
                SAM3_HOST_DEFAULT,
                SAM3_PORT_DEFAULT,
                SAM3_DESTINATION_TRANSIENT,
                SAM3_SESSION_STREAM,
                EdDSA_SHA512_Ed25519, NULL) < 0) {
        log_print(ERROR, "Failed to create session.");
        return false;
    }


    FILE* pk_file = fopen("public_key", "wb");
    if(!pk_file) {
        log_print(ERROR, "Cant open 'public_key' file for writing.\n");
        printf("SERVER PUBLIC KEY ------------- \n"
                "%s\n"
                "------------------------------\n",
                this->session.pubkey);
    }
    else {
        fwrite(this->session.pubkey,
                strlen(this->session.pubkey),
                1,
                pk_file);
        fclose(pk_file);
        printf("Public key written to file.\n");
    }


    printf("Ready.\n");
    m_accept_clients_th = std::thread(&Server::m_accept_clients_th__func, this);
    m_packetrecv_th = std::thread(&Server::m_packetrecv_th__func, this);


    // TEMPORARY FOR TESTING PURPOSES.
    while(true) {

        printf("> ");
        fflush(stdout);

        char buffer[16+1] = { 0 };
        size_t read_len = read(1, buffer, 16);

        if(read_len > 0) {
            if(strcmp(buffer, "end\n") == 0) {
                break;
            }
            else
            if(strcmp(buffer, "online\n") == 0) {
                m_clients_mutex.lock();
                printf("Clients online: %li / %i\n", m_clients.size(), m_config.max_clients);
                m_clients_mutex.unlock();
            }
            else
            if(strcmp(buffer, "msgs\n") == 0) {
                printf("msgs: no messages.\n");
            }
            else
            if(strcmp(buffer, "test\n") == 0) {
            }
            else {
                printf("Unknown command. Commands are:\n"
                        "end     :  shutdown server.\n"
                        "online  :  number of clients online.\n"
                        "msgs    :  see messages.\n"
                        "test    :  send test packet\n"
                        );
            }
        }
    }

    return true;
}

void Server::stop() {
    printf("Server shutdown...\n");
    printf("Closing threads...\n");

    m_threads_running = false;

    m_accept_clients_th.join(); printf("accept_clients_th - finished.\n");
    m_packetrecv_th.join();     printf("packetrecv_th - finished.\n");

    sam3CloseSession(&this->session);
}
    

#include <poll.h>

void Server::m_packetrecv_th__func() {
    std::string msg = "";
    msg.reserve(1024);

    int test = 0;
    while(m_threads_running) {

        m_clients_mutex.lock();

        for(size_t i = 0; i < m_clients.size(); i++) {
         
            ServerClient* client = &m_clients[i];
            if(client->conn == NULL) {
                printf("INVALID CLIENT\n");
                continue;
            }

            struct pollfd pfd;
            pfd.fd = client->conn->fd;
            pfd.events = POLLIN;
            
            int poll_result = poll(&pfd, (nfds_t)1, 100);
            if(poll_result <= 0) {
                printf("\033[90m...\033[0m\n");
                continue;
            }

            PacketID p_id = PacketID::EMPTY;
            msg.clear();

            if(!Util::receive_data(client->conn->fd, &p_id, &msg)) {
                continue;
            }
            

            printf("%s: %x : '%s'\n",
                    __func__, p_id, msg.c_str());

            switch(p_id) {
                case PacketID::LEAVING:
                    sam3CloseConnection(client->conn);
                    client->conn = NULL;
                    m_clients.erase(m_clients.begin() + i);
                    i--;
                    //Util::send_packet(client->conn->fd, PacketID::LEAVE_OK);
                    break;

                case PacketID::MESSAGE:
                    break;
            }
        }

        printf("online: %li\n", m_clients.size());
        m_clients_mutex.unlock();

        std::this_thread::sleep_for(
                std::chrono::milliseconds(1000));
    }
}




// MOVE THIS......

static inline void strcpyerr(Sam3Session *ses, const char *errstr) {
  memset(ses->error, 0, sizeof(ses->error));
  if (errstr != NULL)
    strncpy(ses->error, errstr, sizeof(ses->error) - 1);
}
static int sam3CloseConnectionInternal(Sam3Connection *conn) {
  return (conn->fd >= 0 ? sam3tcpDisconnect(conn->fd) : 0);
}

Sam3Connection *sam3StreamAccept_timeout(Sam3Session *ses, int timeout_ms) {
  if (ses != NULL) {
    SAMFieldList *rep = NULL;
    char repstr[1024];
    Sam3Connection *conn;
    struct pollfd pfd;
    int poll_result;
    //
    if (ses->type != SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return NULL;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return NULL;
    }
    if ((conn = (Sam3Connection*)calloc(1, sizeof(Sam3Connection))) == NULL) {
      strcpyerr(ses, "NO_MEMORY");
      return NULL;
    }
    if ((conn->fd = sam3HandshakeIP(ses->ip, ses->port)) < 0) {
      strcpyerr(ses, "IO_ERROR_SK");
      goto error;
    }
    if (sam3tcpPrintf(conn->fd, "STREAM ACCEPT ID=%s\n", ses->channel) < 0) {
      strcpyerr(ses, "IO_ERROR_PF");
      goto error;
    }

    if ((rep = sam3ReadReply(conn->fd)) == NULL) {
      strcpyerr(ses, "IO_ERROR_RP");
      goto error;
    }

    if (!ses->silent) {
      if (!sam3IsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
        const char *v = sam3FindField(rep, "RESULT");
        //
        strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR_RES"));
        goto error;
      }
    }
    
    pfd.fd = conn->fd;
    pfd.events = POLLIN;
    poll_result = poll(&pfd, (nfds_t)1, timeout_ms);

    if(poll_result > 0) {
        if (sam3tcpReceiveStr(conn->fd, repstr, sizeof(repstr)) < 0) {
          strcpyerr(ses, "IO_ERROR_RP1");
          goto error;
        }
    }
    else {
        strcpyerr(ses, "IO_TIMEDOUT");
        goto error;
    }

    sam3FreeFieldList(rep);
    if ((rep = sam3ParseReply(repstr)) != NULL) {
      const char *v = sam3FindField(rep, "RESULT");
      //
      strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR_RES1"));
      goto error;
    }
    if (!sam3CheckValidKeyLength(repstr)) {
      strcpyerr(ses, "INVALID_KEY");
      goto error;
    }
    sam3FreeFieldList(rep);
    strcpy(conn->destkey, repstr);
    conn->ses = ses;
    conn->next = ses->connlist;
    ses->connlist = conn;
    strcpyerr(ses, NULL);
    return conn;
  error:
    if (rep != NULL) {
        sam3FreeFieldList(rep);
    }
    if(conn != NULL) {
        sam3CloseConnectionInternal(conn);
        free(conn);
    }
    return NULL;
  }
  return NULL;
}


bool Server::m_verify_client_name(Sam3Connection* conn, const std::string& name) {
    bool is_name_ok = false;

    if(name.size() > 24) {
        Util::send_packet(conn->fd, 
                PacketID::NICKNAME_BAD, "Name is too long.");
        goto out;
    }
    else
    if(name.empty()) {
        Util::send_packet(conn->fd, 
                PacketID::NICKNAME_BAD, "Name must not be empty.");
        goto out;
    }

    // Check all bytes are printable.
    for(size_t i = 0; i < name.size(); i++) {
        char b = name[i];
        if((b < 0x20) || (b > 0x7E)) {
            Util::send_packet(conn->fd, PacketID::NICKNAME_BAD,
                    "Name must have only printable bytes.");
            goto out;
        }
    }

    is_name_ok = true;
    Util::send_packet(conn->fd, PacketID::NICKNAME_OK);

out:
    return is_name_ok;
}


void Server::m_accept_clients_th__func() {
    while(m_threads_running) {
        /*
        m_threads_exit_mutex.lock();
        if(m_threads_exit) {
            m_threads_exit_mutex.unlock();
            break;
        }
        m_threads_exit_mutex.unlock();
        */

        m_clients_mutex.lock();
        Sam3Connection* conn = sam3StreamAccept_timeout(&this->session, 2000);
        if(conn != NULL) {

            // On client side, sam3StreamConnection is waiting for response.
            sam3tcpPrintf(conn->fd, "welcome client\n");

            // Ask for nickname.
            ServerClient client(conn);
            Util::send_packet(conn->fd, PacketID::ASKING_NICKNAME);

            PacketID resp_id = PacketID::EMPTY;
            std::string client_name = "Unknown";
            if(Util::receive_data(conn->fd, &resp_id, &client_name)
            && (resp_id == PacketID::NICKNAME)) {
                if(!m_verify_client_name(conn, client_name)) {
                    sam3CloseConnection(conn); 
                    goto skip_client;
                }

                printf("'%s' connected.\n", client_name.c_str());

                client.name = client_name;
                m_clients.push_back(client);
            }
            else {
                log_print(ERROR, "Failed to get client nickname.");
                sam3CloseConnection(conn);
            } 

            /*
            m_clients_mutex.lock();
            m_clients.puch_back(client);
            m_clients_mutex.unlock();
            */
            // Pass connection to another thread.
        }
        // else: timeout or error.

skip_client:
        m_clients_mutex.unlock();
        std::this_thread::sleep_for(
                std::chrono::milliseconds(1000));
    }
}

