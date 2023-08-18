#include <bits/stdc++.h>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "json.hpp"
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

void print_book(json book, char ID[]) {
    cout << book.dump(4) << '\n';
}

bool checkNumber(char id[]) {
    for (size_t i = 0; i < strlen(id); i++)
        if (!(id[i] >= '0' && id[i] <= '9'))
            return 0;
    return 1;
}

bool checkSpaces(char a[]) {
    for (size_t i = 0; i < strlen(a); i++)
        if (a[i] == ' ')
            return 0;
    return 1;
}


int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    
    // buffers for cookie and token
    char **cookies = (char **)calloc(1, sizeof(char *));
    cookies[0] = (char *)calloc(1000, sizeof(char *));

    char *token = (char *)calloc(1000, sizeof(char *));

    while (1) {

        char command[100];
        memset(command, 0, sizeof(command));
        cin.getline(command, 100);

        if (strcmp(command, "register") == 0) {

            // reading username and password to register
            char username[100], pass[100];
            memset(username, 0, sizeof(username));
            memset(pass, 0, sizeof(pass));
            cout << "username=";
            cin.getline(username, 100);
            if (!checkSpaces(username)) {
                cout << "Username cannot contain spaces\n";
                continue;
            }

            cout << "password=";
            cin.getline(pass, 100);
            if (!checkSpaces(pass)) {
                cout << "Password cannot contain spaces\n";
                continue;
            }

            // creating json object
            json form_data = {{"username", username}, {"password", pass}};

            // making the json a char* to pass into the function
            char **data = (char **)calloc(1, sizeof(char *));
            data[0] = (char *)calloc(1000, sizeof(char *));
            strcpy(data[0], form_data.dump().c_str());
            
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);

            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }
            // post request for the new account
            message = compute_post_request("34.254.242.81:8080", "/api/v1/tema/auth/register", "application/json", data, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                // if there is an error print the error message from the server
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                // registration is done
                cout << "Register successful!" << '\n';
            }

        } else if (strcmp(command, "login") == 0) {
            // reading username and password to login
            char username[100], pass[100];
            memset(username, 0, sizeof(username));
            memset(pass, 0, sizeof(pass));
            cout << "username=";
            cin.getline(username, 100);
            if (!checkSpaces(username)) {
                cout << "Username cannot contain spaces\n";
                continue;
            }

            cout << "password=";
            cin.getline(pass, 100);
            if (!checkSpaces(pass)) {
                cout << "Password cannot contain spaces\n";
                continue;
            }

            json form_data = {{"username", username}, {"password", pass}};

            char **data = (char **)calloc(1, sizeof(char *));
            data[0] = (char *)calloc(1000, sizeof(char *));
            strcpy(data[0], form_data.dump().c_str());
            
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_post_request("34.254.242.81:8080", "/api/v1/tema/auth/login", "application/json", data, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                // we received a cookie and we need to keep it for this session for further requests
                char *setCookie = strstr(response, "Set-Cookie");
                if (setCookie) {
                    strcpy(cookies[0], setCookie + 12);
                    char *Date = strstr(cookies[0], "Date");
                    *(Date - 2) = '\0';
                }
                // login is done
                cout << "Login successful!" << '\n';
            }
        } else if (strcmp(command, "enter_library") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }

            // sending a get request with the session cookie to get authorization for the library
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_get_request("34.254.242.81:8080", "/api/v1/tema/library/access", NULL, cookies, 1, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                // we received a token and as for the cookie we need to keep it for further operations
                char *getToken = strstr(response, "token");
                if (getToken) {
                    strcpy(token, getToken + 8);
                    token[strlen(token) - 2] = '\0';
                    cout << "Access granted" << '\n';
                }
            }
        } else if (strcmp(command, "get_books") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }

            if (strlen(token) == 0) {
                cout << "You need to enter the library first!\n";
                continue;
            }
            // sending a get request with the jwt token to list all our books
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_get_request("34.254.242.81:8080", "/api/v1/tema/library/books", NULL, NULL, 1, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");

            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                // getting the books from the response
                char* bookList = strstr(response, "[");
                json j = json::parse(bookList);
                int nr = 0;
                for (auto it: j) {
                    nr++;
                    print_book(it, nullptr);
                }

                if (nr == 0) {
                    cout << "No books found\n";
                }
            }
        } else if (strcmp(command, "get_book") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }
            
            if (strlen(token) == 0) {
                cout << "You need to enter the library first!\n";
                continue;
            }
            char id[30];
            memset(id, 0, sizeof(id));
            cout << "id=";
            cin >> id;
            cin.get();

            if (!checkNumber(id)) {
                cout << "id must be a number\n";
                continue;
            }

            char path[100] = "/api/v1/tema/library/books/";
            strcat(path, id);

            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_get_request("34.254.242.81:8080", path, NULL, NULL, 1, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                char *book;
                book = basic_extract_json_response(response);
                json j = json::parse(book);
                print_book(j, id);
            }

        } else if (strcmp(command, "add_book") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }
            
            if (strlen(token) == 0) {
                cout << "You need to enter the library first!\n";
                continue;
            }
            char title[100], author[100], genre[100], page_count[10], publisher[100];
            memset(title, 0, sizeof(title));
            memset(author, 0, sizeof(author));
            memset(genre, 0, sizeof(genre));
            memset(page_count, 0, sizeof(page_count));
            memset(publisher, 0, sizeof(publisher));
            cout << "title=";
            cin.getline(title, 100);

            cout << "author=";
            cin.getline(author, 100);

            cout << "genre=";
            cin.getline(genre, 100);

            cout << "page_count=";
            cin.getline(page_count  , 100);

            cout << "publisher=";
            cin.getline(publisher, 100);
            if (!checkNumber(page_count)) {
                cout << "Page count must be a number\n";
                continue;
            }
            json form_data = {{"title", title}, {"author", author}, {"genre", genre}, {"page_count", page_count}, {"publisher", publisher}};

            char **data = (char **)calloc(1, sizeof(char *));
            data[0] = (char *)calloc(1000, sizeof(char *));
            strcpy(data[0], form_data.dump().c_str());
            
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_post_request("34.254.242.81:8080", "/api/v1/tema/library/books", "application/json", data, 1, NULL, 0, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                cout << "Book successfully added\n";
            }
        } else if (strcmp(command, "delete_book") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }
            
            if (strlen(token) == 0) {
                cout << "You need to enter the library first!\n";
                continue;
            }

            char id[30];
            memset(id, 0, sizeof(id));
            cout << "id=";
            cin >> id;
            cin.get();

            if (!checkNumber(id)) {
                cout << "id must be a number\n";
                continue;
            }

            char path[100] = "/api/v1/tema/library/books/";
            strcat(path, id);

            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }

            message = compute_delete_request("34.254.242.81:8080", path, NULL, NULL, 1, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                cout << "Book successfully deleted" << '\n';
            }

        } else if (strcmp(command, "logout") == 0) {
            if (strlen(cookies[0]) == 0) {
                cout << "You need to login first!\n";
                continue;
            }
            
            sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cout << "Connection failed, please try again\n";
                continue;
            }
            
            message = compute_get_request("34.254.242.81:8080", "/api/v1/tema/auth/logout", NULL, cookies, 1, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            char *err = strstr(response, "error");
            if (err) {
                char error[100];
                memset(error, 0, sizeof(error));
                strcpy(error, err + 8);
                error[strlen(error) - 2] = '\0';
                cout << error << "\n";
            } else {
                strcpy(token, "");
                strcpy(cookies[0], "");
                cout << "Logout successful\n";
            }
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            cout << "Invalid command\n";
        }

    }

    return 0;
}
