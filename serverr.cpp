//
//  Server.cpp
//  Chatting Application
//
//  Created by Seif elmougy on 17/04/2024.
//

#include <stdio.h>
#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock.h>
#pragma comment(lib, "WS2_32.lib")
#include <fstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <thread>

using namespace std;

// Caesar Cipher Encryption and Decryption Functions
string encryptMessage(const std::string& message, int key) {
    string encryptedMessage = message;
    for (char& c : encryptedMessage) {
        if (isalpha(c)) {
            char base = isupper(c) ? 'A' : 'a';
            c = (c - base + key) % 26 + base;
        }
    }
    return encryptedMessage;
}

string decryptMessage(const string& encryptedMessage, int key) {
    return encryptMessage(encryptedMessage, 26 - key);
}

namespace pt = boost::property_tree;

// User Structure to Store User Information
struct User {
    string username;
    string password;
    bool online;
    SOCKET socket;
};

// Map to Store User Information
map<string, User> users;

// Function to store user credentials in a text file
void storeCredentials(const string& username, const string& hashedPassword) {
    ofstream file("credentials.txt", ios::app);
    if (file.is_open()) {
        file << username << ":" << hashedPassword << endl;
        file.close();
    }
    else {
        cerr << "Error: Unable to open credentials file for writing" << endl;
    }
}

// Function to check if a username is available (not already taken)
bool isUsernameAvailable(const std::string& username) {
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
string getHashedPassword(const std::string& username) {
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

// Function to send response to client
void sendResponse(SOCKET clientSocket, const pt::ptree& response) {
    // Convert PropertyTree response to JSON string
    stringstream ss;
    pt::write_json(ss, response);
    string responseStr = ss.str();

    // Send response to client
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
}

// Function to handle account creation request
void handleAccountCreation(SOCKET clientSocket, const pt::ptree& request) {
    string username = request.get<string>("username");
    string password = request.get<string>("password");
    // Hash the password securely using a cryptographic hash function (e.g., SHA-256)
    string hashedPassword = "hashed_password"; // Placeholder for actual hashing
    if (!isUsernameAvailable(username)) {
        pt::ptree response;
        response.put("status", "failure");
        response.put("message", "Username already taken");
        sendResponse(clientSocket, response);
    }
    else {
        storeCredentials(username, hashedPassword);
        User user;
        user.username = username;
        user.password = password; // Store password in memory for simplicity
        user.online = false;
        user.socket = clientSocket;
        users[username] = user;
        pt::ptree response;
        response.put("status", "success");
        response.put("message", "Account created successfully");
        sendResponse(clientSocket, response);
    }
}

// Function to handle authentication request
void handleAuthentication(SOCKET clientSocket, const pt::ptree& request) {
    string username = request.get<string>("username");
    string password = request.get<string>("password");
    string storedHashedPassword = getHashedPassword(username);
    // Hash the provided password
    string hashedPassword = "hashed_password"; // Placeholder for actual hashing
    if (storedHashedPassword == hashedPassword) {
        // Mark user as online
        users[username].online = true;
        users[username].socket = clientSocket;
        pt::ptree response;
        response.put("status", "success");
        response.put("message", "Authentication successful");
        sendResponse(clientSocket, response);
    }
    else {
        pt::ptree response;
        response.put("status", "failure");
        response.put("message", "Authentication failed: Invalid username or password");
        sendResponse(clientSocket, response);
    }
}

// Function to handle chat message request
void handleChatMessage(SOCKET clientSocket, const pt::ptree& request) {
    string sender = request.get<string>("sender");
    string receiver = request.get<string>("receiver");
    string message = request.get<string>("message");
    // Check if the receiver is online
    if (users.find(receiver) != users.end() && users[receiver].online) {
        // Encrypt message using Caesar cipher
        int key = 3; // Caesar cipher key
        string encryptedMessage = encryptMessage(message, key);
        // Prepare response
        pt::ptree response;
        response.put("status", "success");
        response.put("sender", sender);
        response.put("receiver", receiver);
        response.put("message", encryptedMessage);
        // Send encrypted message to receiver
        sendResponse(users[receiver].socket, response);
    }
    else {
        pt::ptree response;
        response.put("status", "failure");
        response.put("message", "Receiver is not online");
        sendResponse(clientSocket, response);
    }
}

// Function to handle client requests
void handleClientRequest(SOCKET clientSocket) {
    // Receive data from the client
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    // Parse received data as PropertyTree
    pt::ptree request;
    stringstream ss(buffer);
    pt::read_json(ss, request);
    // Handle different types of requests
    string command = request.get<string>("command");
    if (command == "create_account") {
        handleAccountCreation(clientSocket, request);
    }
    else if (command == "authenticate") {
        handleAuthentication(clientSocket, request);
    }
    else if (command == "chat_message") {
        handleChatMessage(clientSocket, request);
    }
    else {
        // Invalid command
        pt::ptree response;
        response.put("status", "failure");
        response.put("message", "Invalid command");
        sendResponse(clientSocket, response);
    }
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Error initializing Winsock\n";
        return 1;
    }
    // Create a TCP socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Error creating socket\n";
        WSACleanup();
        return 1;
    }
    // Bind the socket to an address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080); // Port number
    if (::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error binding socket\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    // Listen for incoming connections
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        cerr << "Error listening for incoming connections\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    cout << "Server is running and listening on port 8080...\n";
    // Accept incoming connections
    struct sockaddr_in clientAddress;
    int clientAddressLength = sizeof(clientAddress);
    while (true) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Error accepting connection\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }
        cout << "New connection accepted.\n";
        // Handle client requests in a separate thread
        thread([&]() {
            handleClientRequest(clientSocket);
            // Close the client socket
            closesocket(clientSocket);
            cout << "Connection closed.\n";
            }).detach();
    }
    // Close the server socket
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}



// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
