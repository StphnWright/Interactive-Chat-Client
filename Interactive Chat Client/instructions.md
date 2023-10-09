# Chat Client

## Overview

The goal of this assignment is to create a chat client that is able to communicate
with a server. The client will send messages to the server, which is then able to
broadcast the message to all the other clients on the system. Similarly, the
client will be able to receive messages directly from the server, thus enabling
the user to read messages sent by all other clients on the system. 

You should generate an executable called `chatclient`.
## Part 0

You will be hosting your server on this IP address: 127.0.0.1 which will listen and wait for connections on a port. For this part, you will just do a little math to find the port numbers that you should use in your testing. This does not mean that your client will **only** be connecting to servers listening for connections on ports in your port number range, but this is just to reduce collision of all the students testing their client on our AP server. 

(1) Log into the server and run the `id -u` command to find your user ID on the AP server. Take the last three digits:
```
$ id -u
1003
```
Take the last three digits: 003

(2) Pick a number, x, between 10 and 63 (inclusive) and use this function to optain your port number: 

`1000 * x + (the last three digits of your user ID)`

For me, I can use any of these ports: 10003, 21003, 62003, etc.
    
Using this scheme, we assign 54 ports to each student without overlap. It may take some time for a port to become available after you just tested with it, so rotate through your available ports when testing. 


## Part 1 

This program requires two command-line arguments: server IP and port. If no
arguments have been supplied, the usage message should be printed 
to stderr and you should return `EXIT_FAILURE`.

    "Usage: %s <server IP> <port>\n"

The IPv4 address should be written in dot-decimal notation: a.b.c.d, where
a, b, c, and d can be any integer 0 through 255. If you are running the server
locally, you specify the address as 127.0.0.1. Do not worry about getting the word
localhost to work. Use `inet_pton()` to check the IP address for validity. If
the IP address is invalid, you should print a reasonable error message 
that includes the IP address and exit in failure.

The port must be an integer the range [1024, 65535]. If it is not, you should
print a reasonable error message and exit in failure.

## Part 2
After parsing the command-line arguments, the program must prompt the user for a username.
If the length of the username is 0, the program should reprompt. The same holds if it
is too long, in which case the program outputs

    "Sorry, limit your username to %d characters.\n"

where %d is `MAX_NAME_LEN` and then reprompts the user. Do not worry about the case where
the user enters only spaces. Do not worry about trimming leading and trailing whitespace
in the username either.

After obtaining a user name, print

    "Hello, %s. Let's try to connect to the server.\n"

where %s is the username.

## Part 3

After obtaining the user name, which may actually be the same as someone already logged into the system, 
your program will take the following steps.

1. Create a TCP Socket
2. Connect to the server
3. Receive the welcome message from the server

    If the number of bytes received is 0, that means the server closed
    the connection on the client and you should print an error message and exit in failure.

    *Note: You should store all messages received from the socket in
    inbuf an produce all outbound messages from STDIN in outbuf. Null-terminate all strings
    before sending them to make processing input easier on the server. The server null-terminates
    all strings sent to the client.*

4. Next, print a new line, the welcome message received from the server, and two more lines.

5. Send the username to the server.

If any of these steps fail, you need to print an error message (that includes the strerror of errno) 
to stderr and exit in failure.

## Part 4

Using `fd_set` and `select` (man 2 select), your program should now loop forever, 
prompting the user for input and determining if there is activity on of the two
file descriptors. Specifically, both the socket and `STDIN_FILENO` should be
added to the `fd_set`. If there is activity on

1. `STDIN_FILENO`

    Your chat client should work if the user types directly into the terminal or if a file is redirected
    through stdin. If using file redirection, the first line will be the username. The rest of the lines
    will each contain a message to be sent to the server. A new line marks the end of a message and must
    be sent before reading additional characters from the file. If a message is too long (no new line
    character is found before visiting MAX_MSG_LEN characters, print to standard error:

        "Sorry, limit your message to 1 line of at most %d characters.\n"
    
    where %d is `MAX_MSG_LEN`. Consume the rest of the characters up to EOF or a new line character, so that
    message is discarded.

    Otherwise, send the message to the server, regardless of its contents.
    Then, if the message is equal to "bye", print "Goodbye.\n" and shut down the client.

2. Client Socket

    Receive data from the socket and store it in inbuf. If the number
    of bytes received is -1 and errno != EINTR, warn the user with the message:

        "Warning: Failed to receive incoming message."
    
    If the number of bytes received is 0, the server abruptly broke the 
    connection with the client, as in the server crashed or perhaps the network failed.
    Error out by telling the user:

        "\nConnection to server has been lost.\n"

    Compare it to "bye" and if it's equal, print "\nServer initiated shutdown.\n" and exit the program.

    Otherwise, print the message from the server.

Make sure you close the socket before your program terminates. Be
sure to check for memory leaks as well.

## Testing your code

We have provided the code for the chat server in `chatserver.c`. The code for the server is complete, so you should not make any changes to the code. But go through it and try to understand it, this will really help with coding the client. Additionally, the server code is how you can go about testing your client. Here is how to do it, first open up two windows: 

1. Add chatserver to your Makefile so that you can create the executable.
2. In one window, start your `chatserver` by running `./chatserver <port-number>`. Remember to use an appropriate port number to reduce collision with other students. 
3. Then, on your second window, build your `chatclient` and run using `./chatclient 127.0.0.1 <port-number>`. This should be the port number that you supplied to `chatserver`. 

Like all homeworks, you should be coding and testing your implementation incrementally. You can test at different stages, but the first thing you want to do is make sure you are establishing the connection between the server and the client. After, comes the other parts like verifying the username length and the error checking. But, in order for you to test those parts you have to have your server-client connection set up! Per usual, if you have questions please post on the Ed or go to OH. 
