# Chess

A chess game written in C++. Only for Windows OS for now.

There is 2 binaries in this project : a client and a server.

You can play alone on your computer against the AI using only the client.
You can play against someone else if you both use the client to connect to the server someone need to host.

## Demo

https://youtu.be/o4DzCjSoBsc

## Client

Used to play alone on your computer against the IA.
For multiplayer you need to connect to a server. You can specify the server used by the client in the server.ini file in the Client directory.
The client need a username and a password to log into the server.

## Server

Start a new chess game with the 2 first persons asking to play multiplayer.

The server need to access a database (MariaDB or MYSQL) where the usernames and passwords are stored. You need a scheme named 'chess' with a table 'players' and at least 2 columns : 'login' and 'pwd'. For now you have to manually create users in the database.

### Configuration

In Server directory :
- db.ini : db access configuration
- server.ini : port used by the server to listen for connections by the clients
