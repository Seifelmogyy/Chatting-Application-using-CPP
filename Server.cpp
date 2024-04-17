//
//  Server.cpp
//  Chatting Application
//
//  Created by Seif elmougy on 17/04/2024.
//

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>

using namespace std;
using json = nlohmann::json;


// Function to store user credentials in a text file
void storeCredentials(const string& username, const string& hashedPassword) {
    ofstream file("credentials.txt", ios::app);
    if (file.is_open()) {
        file << username << ":" << hashedPassword << endl;
        file.close();
    } else {
        cerr << "Error: Unable to open credentials file for writing" << endl;
    }
}


// Function to check if a username is available (not already taken)
bool isUsernameAvailable(const string& username) {
    ifstream file("credentials.txt");
    string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string storedUsername = line.substr(0, pos);
            if (storedUsername == username) {
                return false; // Username already taken
            }
        }
    }
    return true; // Username available
}


// Function to retrieve the hashed password associated with a username
string getHashedPassword(const string& username) {
    ifstream file("credentials.txt");
    string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string storedUsername = line.substr(0, pos);
            if (storedUsername == username) {
                return line.substr(pos + 1); // Return hashed password
            }
        }
    }
    return ""; // Username not found
}


// Function to handle account creation request
void handleAccountCreation(int clientSocket, const json& request) {
    string username = request["username"];
    string password = request["password"];
    // Hash the password securely using a cryptographic hash function (e.g., SHA-256)
    string hashedPassword = "hashed_password"; // Placeholder for actual hashing
    if (!isUsernameAvailable(username)) {
        json response = {{"status", "failure"}, {"message", "Username already taken"}};
        sendResponse(clientSocket, response);
    } else {
        storeCredentials(username, hashedPassword);
        json response = {{"status", "success"}, {"message", "Account created successfully"}};
        sendResponse(clientSocket, response);
    }
}


// Function to handle authentication request
void handleAuthentication(int clientSocket, const json& request) {
    string username = request["username"];
    string password = request["password"];
    string storedHashedPassword = getHashedPassword(username);
    // Hash the provided password
    string hashedPassword = "hashed_password"; // Placeholder for actual hashing
    if (storedHashedPassword == hashedPassword) {
        json response = {{"status", "success"}, {"message", "Authentication successful"}};
        sendResponse(clientSocket, response);
    } else {
        json response = {{"status", "failure"}, {"message", "Authentication failed: Invalid username or password"}};
        sendResponse(clientSocket, response);
    }
}


void sendResponse(int clientSocket, const json& response) {
    // Convert JSON response to string
    string responseStr = response.dump();
    // Send response to client
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
}



int main() {
    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Error creating socket\n";
        return 1;
    }

    // Bind the socket to an address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080); // Port number

    if (::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error binding socket\n";
        return 1;
    }

    // Listen for incoming connections
    listen(serverSocket, 5);

    // Accept incoming connections
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (clientSocket < 0) {
        cerr << "Error accepting connection\n";
        return 1;
    }

    // Send data to the client
    const char* message = "Hello from server";
    send(clientSocket, message, strlen(message), 0);
    
    // Receive data from the client
        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        // Parse received data as JSON
        json request = json::parse(buffer);

        // Handle different types of requests
        string command = request["command"];
        if (command == "create_account") {
            handleAccountCreation(clientSocket, request);
        } else if (command == "authenticate") {
            handleAuthentication(clientSocket, request);
        } else {
            // Invalid command
            json response = {{"status", "failure"}, {"message", "Invalid command"}};
            sendResponse(clientSocket, response);
        }

    // Close the sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}
