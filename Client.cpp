//
//  Client.cpp
//  Chatting Application
//
//  Created by Seif elmougy on 17/04/2024.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;


// Function to send request to server
void sendRequest(int serverSocket, const json& request) {
    // Convert JSON request to string
    string requestStr = request.dump();
    // Send request to server
    send(serverSocket, requestStr.c_str(), requestStr.length(), 0);
}


// Function to receive response from server
json receiveResponse(int serverSocket) {
    // Receive response from server
    char buffer[1024] = {0};
    recv(serverSocket, buffer, sizeof(buffer), 0);
    // Parse response as JSON
    return json::parse(buffer);
}



int mainnn() {
    // Create a TCP socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error creating socket\n";
        return 1;
    }

    // Connect to the server
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address
    serverAddress.sin_port = htons(8080); // Server port number
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error connecting to server\n";
        return 1;
    }

    // Receive data from the server
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from server: " << buffer << std::endl;

    // Close the socket
    close(clientSocket);

    return 0;
}
