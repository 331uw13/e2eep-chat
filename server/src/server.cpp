#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <poll.h>
#include <openssl/rand.h>

#include <cstring>
#include <chrono>

#include "server.hpp"
#include "../../shared/log.hpp"


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
            else {
                printf("Unknown command. Commands are:\n"
                        "end     :  shutdown server.\n"
                        "online  :  number of clients online.\n"
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
    


void Server::m_packetrecv_th__func() {
    //std::string msg = "";
    //msg.reserve(1024);

    Packet packet;

    while(m_threads_running) {

        m_clients_mutex.lock();

        for(size_t i = 0; i < m_clients.size(); i++) {
         
            ServerClient* client = &m_clients[i];
            if(client->conn == NULL) {
                printf("INVALID CLIENT\n");
                continue;
            }

            if(!Util::socket_ready_inms(client->conn->fd, 20, Util::TimeoutAct::DO_NOTHING)) {
                continue;
            }


            packet.clear();
            if(!Util::server_receive(client->conn->fd, &packet)) {
                log_print(ERROR, "RECEIVE ERROR!");
                continue;
            }


            if(!is_good_packet(packet)) {
                continue;
            }

            switch(packet.id) {
                case PacketID::LEAVING:
                    printf("'%s' disconnected.\n", client->name.c_str());
                    sam3CloseConnection(client->conn);
                    client->conn = NULL;
                    m_clients.erase(m_clients.begin() + i);
                    i--;
                    break;

                case PacketID::MESSAGE:
                    if(is_safe_msg(packet.data)) {
                        //printf("|%s|\n", packet.data.c_str());
                        std::string msg = "(" + packet.nickname + "): " + packet.data;
                        broadcast(msg);
                    }
                    break;
            }
        }
        m_clients_mutex.unlock();

        std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
    }
}


void Server::broadcast(const std::string& msg) {
    for(const ServerClient& sc : m_clients) {
        if(!sc.conn) {
            continue;
        }
        if(sc.conn->fd < 0) {
            continue;
        }
        Util::send_simple_packet(sc.conn->fd, PacketID::MESSAGE, msg.c_str());
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
        Util::send_simple_packet(conn->fd, 
                PacketID::NICKNAME_BAD, "Name is too long.");
        goto out;
    }
    else
    if(name.empty()) {
        Util::send_simple_packet(conn->fd, 
                PacketID::NICKNAME_BAD, "Name must not be empty.");
        goto out;
    }

    // Check all bytes are printable.
    for(size_t i = 0; i < name.size(); i++) {
        char b = name[i];
        if((b < 0x20) || (b > 0x7E)) {
            Util::send_simple_packet(conn->fd, PacketID::NICKNAME_BAD,
                    "Name must have only printable bytes.");
            goto out;
        }
    }


    // Everything should be fine now
    // generate session token for the client and continue.

    token_t session_token;
    if(!RAND_bytes(session_token.data(), session_token.size())) {
        Util::send_simple_packet(conn->fd, PacketID::NICKNAME_BAD,
                "Sorry, failed to create session token. Try reconnecting.");
        goto out;
    }

    m_name_token_map.insert(std::make_pair(name, session_token));

    Util::send_simple_packet(conn->fd, PacketID::NICKNAME_OK, 
            std::string(std::begin(session_token), std::end(session_token)));

    is_name_ok = true;
out:
    return is_name_ok;
}


void Server::m_accept_clients_th__func() {
    while(m_threads_running) {

        Sam3Connection* conn = sam3StreamAccept_timeout(&this->session, 200);
        if(conn != NULL) {

            // On client side, sam3StreamConnection is waiting for response.
            sam3tcpPrintf(conn->fd, "welcome client\n");

            // Ask for nickname.
            ServerClient client(conn);
            Util::send_simple_packet(conn->fd, PacketID::ASKING_NICKNAME);

            PacketID resp_id = PacketID::EMPTY;
            std::string client_name = "Unknown";
            if(Util::receive_idnmsg(conn->fd, &resp_id, &client_name)
            && (resp_id == PacketID::NICKNAME)) {
                if(!m_verify_client_name(conn, client_name)) {
                    sam3CloseConnection(conn); 
                    goto skip_client;
                }


                printf("'%s' connected.\n", client_name.c_str());

        
                m_clients_mutex.lock();
                client.name = client_name;
                m_clients.push_back(client);
                m_clients_mutex.unlock();
            }
            else {
                log_print(ERROR, "Failed to get client nickname.");
                sam3CloseConnection(conn);
            } 
        }
        // else: timeout or error.

skip_client:
        std::this_thread::sleep_for(
                std::chrono::milliseconds(200));
    }
}
        
bool Server::is_good_packet(const Packet& packet) {
    if((packet.id < PacketID::EMPTY) || (packet.id >= PacketID::PACKET_MAX)) {
        log_print(ERROR, "INVALID PACKET ID");
        return false;
    }

    if(packet.nickname.size() >= NICKNAME_MAX) {
        log_print(ERROR, "TOO LONG NAME");
        return false;
    }

    const auto search_res = m_name_token_map.find(packet.nickname);
    if(search_res == m_name_token_map.end()) {
        log_print(ERROR, "NO TOKEN FOUND");
        return false;
    }

    if(search_res->second != packet.session_token) {
        log_print(ERROR, "INVALID TOKEN");
        return false;
    }


    return true;
}

        
bool Server::is_safe_msg(const std::string& data) {
    for(size_t i = 0; i < data.size(); i++) {
        char c = data[i];
        if((c < 0x20) || (c > 0x7E)) {
            return false;
        }
    }
    return true;
}



