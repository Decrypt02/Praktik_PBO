#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define PORT 8080
#define BUFFER_SIZE 1024
#define FOLDER_DOCUMENT "dokumen/" // Path ke file resource
#define CGI_PATH "./cgi" // Path ke program_cgi
void parse_request_line(
    char *request,
    char *method,
    char *uri,
    char *http_version,
    char *query_string,
    char *post_data) {
        char request_message[BUFFER_SIZE];
        char request_line[BUFFER_SIZE];
        char *words[3];
// Baca baris pertama dari rangkaiandata request
    strcpy(request_message, request);
    char *line = strtok(request_message, "\r\n");
    if (line == NULL) {
    fprintf(stderr, "Error: Invalid request line\n");
    return;
    }
    strcpy(request_line, line);
// Pilah request line berdasarkanspasi
    int i = 0;
    char *token= strtok(request_line, " ");
    while (token!= NULL && i < 3) {
    words[i++] = token;
    token= strtok(NULL, " ");
    }
    // Pastikanada 3 komponendalam request line
    if (i < 3) {
    fprintf(stderr, "Error: Request line tidak lengkap\n");
    return;
    }
// kata 1 : Meth od, kata 2 : uri, kata 3 : versi HTTP
    strcpy(method, words[0]);
    strcpy(uri, words[1]);
    strcpy(http_version, words[2]);
    // Hapus tanda / pada URI
    if (uri[0] == '/') {
    memmove(uri, uri + 1, strlen(uri));
    }
// Cek apakah ada query string
    char *query_start = strchr(uri, '?');
    if (query_start != NULL) {
// Pisah kanquery stringdari URI
    *query_start = '\0';
// Salinquery string
    strcpy(query_string, query_start + 1);
    } else {
// Tidak ada query string
    query_string[0] = '\0';
    }
//Cek apakah ada body data
    char *body_start = strstr(request, "\r\n\r\n");
    if (body_start != NULL) {
// Pindah kanpointer ke awal body data
    body_start += 4; // Melewati CRLF CRLF
// Salindata POST dari body
    strcpy(post_data, body_start);
    } else {
    post_data[0] = '\0'; // Tidak ada body data
    }
// Jika URI kosong, maka isi URI denganresource default
// yaitu index.h tml
    if (strlen(uri) == 0) {
    strcpy(uri, "index.h tml");
    }
}
void get_mime_type(char *file, char *mime) {
// Buat variabel yangmemisah kanextensiondari file
    const char *dot = strrchr(file, '.');
// Jika dot NULL maka isi variabel mime dengan"text.html"
// Jika dot tidak NULL maka isi variabel mime denganMIME type
    if (dot == NULL) strcpy(mime, "text/html");
    else if (strcmp(dot, ".h tml") == 0) strcpy(mime, "text/html");
    else if (strcmp(dot, ".css") == 0) strcpy(mime, "text/css");
    else if (strcmp(dot, ".js") == 0) strcpy(mime, "application/js");
    else if (strcmp(dot, ".jpg") == 0) strcpy(mime, "image/jpeg");
    else if (strcmp(dot, ".png") == 0) strcpy(mime, "image/png");
    else if (strcmp(dot, ".gif") == 0) strcpy(mime, "image/gif");
    else if (strcmp(dot, ".ico") == 0) strcpy(mime, "image/ico");
    else strcpy(mime, "text/html");
    }
void handle_method(
    char **response,
    int *response_size,
    char *method,
    char *uri,
    char *http_version,
    char *query_string,
    char *post_data) {
    char fileURL[256];
    snprintf(
        fileURL,
        sizeof(fileURL),
        "%s%s",
        FOLDER_DOCUMENT,
        uri);
    FILE *file = fopen(fileURL, "rb");
    // Jika file resource tidak ditemukan,
    // maka kirimkanstatus 404 ke browser
    if (!file) {
        char not_found[BUFFER_SIZE];
        snprintf(not_found, sizeof(not_found),
            "%s 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length : %lu\r\n"
            "\r\n"
            "<h 1>404 Not Found</h 1>",
            http_version,
            strlen("<h 1>404 Not Found</h 1>")
        );
        *response = (char *)malloc(strlen(not_found) + 1);
        strcpy(*response, not_found);
        *response_size = strlen(not_found);
    } else {
        char *content;
        size_t content_length ;
    //Jika ekstensi file adalah .ph p
    //maka jalankanCGI
        const char *extension= strrchr(uri, '.');
        if (extension&& strcmp(extension, ".php") == 0) {
    // MenjalankanCGI
        char command[BUFFER_SIZE];
        FILE *fp;
        if ((query_string!= NULL) && (post_data != NULL)) {
            method = "BOTH";
        }
    // Buat perintah untuk menjalankanCGI
    //janganlupa, didepan-- dikasih spasi
        snprintf(command, sizeof(command),
            "%s --target=%s"
            " --method=%s "
            " --data_query_string=\"%s\""
            " --data_post=\"%s\"",
            CGI_PATH, fileURL, method, query_string, post_data);
        fp= popen(command, "r");
        if (fp== NULL) {
            perror("popen");
            exit(EXIT_FAILURE);
        }
    // Baca output dari program CGI
        char cgi_output[BUFFER_SIZE];
        size_t output_len= 0;
        while (
            fgets(cgi_output + output_len,
            sizeof(cgi_output) - output_len, fp) != NULL) {
            output_len+= strlen(cgi_output + output_len);
        }
        pclose(fp);
    // Buat response h eader
        char response_header[BUFFER_SIZE];
        snprintf(response_header, sizeof(response_header),
            "%s 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length : %lu\r\n\r\n",
            http_version, output_len);
    // Alokasikanmemory untuk response
            *response_size = strlen(response_header) + output_len;
            *response = (char *)malloc(*response_size + 1);
            strcpy(*response, response_header);
            strcat(*response, cgi_output);
        }
        else {
    // Jika file resource ditemukan,
    // danekstensi file bukanph p
            char response_line[BUFFER_SIZE];
    // Ambil data MIME type
            char mimeType[32];
            get_mime_type(uri, mimeType);
    // Buat response h eader denganstatus code 200 OK
    // dancontent-type sesuai denganMIME file yangdibaca
        snprintf(response_line, sizeof(response_line),
            "%s 200 OK\r\n"
            "Content-Type: %s\r\n\r\n", http_version, mimeType);
    // Baca file resource yangada di server
            fseek(file, 0, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0, SEEK_SET);
            *response = (char *)malloc(fsize + strlen(response_line) + 1);
            strcpy(*response, response_line);
            *response_size = fsize + strlen(response_line);
    // Alokasikanvar msg_body untuk menampung
    // response h eader danisi file resource
            char *msg_body = *response + strlen(response_line);
            fread(msg_body, fsize, 1, file);
        }
        fclose(file);
        }
    }
        void handle_client(int sock_client) {
        char request[BUFFER_SIZE];
        char *response = NULL;
        int request_size;
        int response_size = 0;
        char method[16], uri[256], query_string[512],
            post_data[512], http_version[16];
        request_size = read(sock_client, request, BUFFER_SIZE - 1);
        if (request_size < 0) {
            perror("Proses baca dari client gagal");
            close(sock_client);
            return;
        }
    // Pastikanstringnull-terminated
        request[request_size] = '\0';
        printf("\n-----------------------------------------------\n");
        printf("Request dari browser :\r\n\r\n%s", request);
        parse_request_line(
            request,
            method,
            uri,
            http_version,
            query_string,
            post_data
        );
        handle_method(
            &response,
            &response_size,
            method,
            uri,
            http_version,
            query_string,
            post_data
        );
        printf("\n-----------------------------------------------\n");
        printf("Method : %s URI : %s\n", method, uri);
        printf("Response ke browser :\n%s\n", response);
        if (response != NULL) {
            if (send(sock_client, response, response_size, 0) < 0) {
                perror("Proses kirim data ke client gagal");
            }
            free(response);
        } else {
            printf("Response data is NULL\n");
        }
        close(sock_client);
    }
int main() {
    int sock_server, sock_client;
    struct sockaddr_in serv_addr;
    int opt = 1;
    int addrlen= sizeof(serv_addr);

    if ((sock_server = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Inisialisasi socket server gagal");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt SO_REUSEADDR gagal");
    close(sock_server);
    exit(EXIT_FAILURE);
}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(
        sock_server,
        (struct sockaddr *)&serv_addr, sizeof(serv_addr)
        ) < 0) {
        perror("proses bind gagal");
        close(sock_server);
        exit(EXIT_FAILURE);
    }
    if (listen(sock_server, 3) < 0) {
        perror("proses listengagal");
        close(sock_server);
        exit(EXIT_FAILURE);
    }
    printf("Server siapIP : %s Port : %d\n", "127.0.0.1", PORT);
    while (1) {
        if ((sock_client = accept(
            sock_server,
            (struct sockaddr *)&serv_addr,
            (socklen_t *)&addrlen)) < 0) {
            perror("proses accept gagal");
            close(sock_server);
            exit(EXIT_FAILURE);
        }
        handle_client(sock_client);
    }
    close(sock_server);
    return 0;
}