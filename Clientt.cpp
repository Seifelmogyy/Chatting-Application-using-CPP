//
//  Client.cpp
//  Chatting Application
//
//  Created by Seif elmougy on 17/04/2024.
//


#include <stdio.h>
#include <iostream>
#include <cstring>
#include <cstdint> // for uintptr_t
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock.h>
#pragma comment(lib, "WS2_32.lib")
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <thread>

namespace pt = boost::property_tree;
using namespace std;


// Caesar Cipher Encryption and Decryption Functions
string encryptMessage(const string& message, int key) {
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

// Function to send request to server
void sendRequest(SOCKET serverSocket, const pt::ptree& request) {
    // Convert PropertyTree request to JSON string
    stringstream ss;
    pt::write_json(ss, request);
    string requestStr = ss.str();
    // Send request to server
    int bytesSent = ::send(serverSocket, requestStr.c_str(), requestStr.length(), 0);
    if (bytesSent == SOCKET_ERROR) {
        cerr << "Error sending request to server: " << WSAGetLastError() << endl;
        // Handle the error, such as closing the socket or returning an error code
    }
}


// Function to receive response from server
pt::ptree receiveResponse(int serverSocket) {
    // Receive response from server
    char buffer[2048] = { 0 };
    int bytesReceived = ::recv(serverSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        cerr << "Error receiving response from server: " << WSAGetLastError() << endl;
        // Handle the error, such as closing the socket or returning an error code
        // For simplicity, we'll throw an exception here
        throw std::runtime_error("Error receiving response from server");
    }
    // Parse response as PropertyTree
    pt::ptree response;
    stringstream ss(buffer);
    pt::read_json(ss, response);
    return response;
}

// Function to listen for incoming messages from other users



int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Error initializing Winsock\n";
        return 1;
    }

    // Create a TCP socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Error creating socket\n";
        WSACleanup();
        return 1;
    }

    // Connect to the server
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address
    serverAddress.sin_port = htons(8080); // Server port number
    if (::connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error connecting to server\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    string username;
    // Display welcome message and menu options
    cout << "Welcome to My Chat Application!" << endl;
    cout << "Choose an option:" << endl;
    cout << "1. Sign up" << endl;
    cout << "2. Sign in" << endl;
    cout << "3. Exit" << endl;

    // Get user input
    int choice;
    cout << "Enter your choice (1, 2, or 3): ";
    cin >> choice;
    try {
        switch (choice) {
        case 1: {
            cout << "You chose to sign up." << endl;
            // Get user input for sign-up (e.g., username, email, password)
            string password;
            cout << "Enter username: ";
            cin >> username;
            cout << "Enter password: ";
            cin >> password;
            // Create sign-up request
            pt::ptree signupRequest;
            signupRequest.put("command", "create_account");
            signupRequest.put("username", username);
            signupRequest.put("password", password);
            // Send sign-up request to server
            sendRequest(clientSocket, signupRequest);
            // Receive and process response from server
            pt::ptree signupResponse = receiveResponse(clientSocket);
            cout << "Sign-up response from server: " << signupResponse.get<string>("status") << endl;
            break;
        }
        case 2: {
            cout << "You chose to sign in." << endl;
            // Get user input for sign-in (e.g., username, password)
            string password;
            cout << "Enter username: ";
            cin >> username;
            cout << "Enter password: ";
            cin >> password;
            // Create sign-in request
            pt::ptree signinRequest;
            signinRequest.put("command", "authenticate");
            signinRequest.put("username", username);
            signinRequest.put("password", password);
            // Send sign-in request to server
            sendRequest(clientSocket, signinRequest);
            // Receive and process response from server
            pt::ptree signinResponse = receiveResponse(clientSocket);
            cout << "Sign-in response from server: " << signinResponse.get<string>("status") << endl;
            break;
        }
        case 3:
            cout << "Exiting the application. Goodbye!" << endl;
            // Close the socket and exit
            closesocket(clientSocket);
            WSACleanup();
            return 0;
        default:
            cout << "Invalid choice. Please choose 1, 2, or 3." << endl;
            // Close the socket and exit
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        // Continue with live chat functionality after authentication
        string receiver;
        
        while (true) {
            cout << "Enter receiver's username (or type 'exit' to quit chat): ";
            cin >> receiver;
            if (receiver == "exit") break;
            string message;
            cout << "Enter message: ";
            cin >> message;
            // Encrypt message using Caesar cipher
            int key = 3; // Caesar cipher key
            string encryptedMessage = encryptMessage(message, key);
            // Create chat message request
            pt::ptree chatMessageRequest;
            chatMessageRequest.put("command", "chat_message");
            chatMessageRequest.put("sender", username); // Assume user is signed in
            chatMessageRequest.put("receiver", receiver);
            chatMessageRequest.put("message", encryptedMessage);
            // Send chat message request to server
            sendRequest(clientSocket, chatMessageRequest);
            // Receive and process response from server
            pt::ptree chatMessageResponse = receiveResponse(clientSocket);
            cout << "Chat message response from server: " << chatMessageResponse.get<string>("status") << endl;

            // Receive messages from other users
            pt::ptree receivedMessage = receiveResponse(clientSocket);
            if (receivedMessage.get<string>("command") == "chat_message") {
                string sender = receivedMessage.get<string>("sender");
                string receivedEncryptedMessage = receivedMessage.get<string>("message");
                // Decrypt received message
                string receivedMessage = decryptMessage(receivedEncryptedMessage, key);
                cout << "Message from " << sender << ": " << receivedMessage << endl;
            }
        }

    }
    catch (const std::exception& e) {
        cerr << "Exception caught: " << e.what() << endl;
    }
    catch (const boost::exception& e) {
        cerr << "Exception caught: "  << endl;
    }
    // Receive data from the server
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from server: " << buffer << endl;

    // Close the socket
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}


// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
