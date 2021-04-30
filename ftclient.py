#!/usr/bin/env python
#CS372 Introduction to Networking, Project 2
#File Transfer Client Created by Matthew Albrecht
#June 2, 2019
#This program is a chat client.  It read the command
#line parameters and either requests a file from the 
#server or it requests the directory from the server.
#Once it makes the request it opens a separate port to
#receive the data it requested.  
#Functions and desctiptions were taken from the following two websites.
#https://docs.python.org/2/howto/sockets.html
#https://docs.python.org/3/library/socket.html

import socket
import select 
import sys 
#from thread import *
TCP_IP = sys.argv[1] + ".engr.oregonstate.edu"

#This function is the startup function.  It makes the inital Connection
#with the server, and reads all of the command line parameters.  It also
#checks the command line parameters for correctness. It takes no parameters.
def startup ():
    TCP_PORT1 = sys.argv[2]
    BUFFER_SIZE = 1024
	#create the socket
    control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	#connect to the socket
    control.connect((TCP_IP, int(TCP_PORT1)))
    print("Connected to server on port: " + TCP_PORT1)
	#check the command line parameters for correctness
    if sys.argv[3] == "-g":
        consoleMessage = sys.argv[5] + " " + sys.argv[3] + " " + sys.argv[4]
        control.send(consoleMessage)
    else:
        if sys.argv[3] == "-l":
            consoleMessage = sys.argv[4] + " " + sys.argv[3]
            control.send(consoleMessage)
        else:
            control.send("5005 -m this.is.an.error.message")
            print("Please enter a valid command")
            control.close()
            exit()
    control.close()
    return control

#This function creates the data port that the Server
#will connect to.  I took this function's layout from
#my first project.
def dataConnection ():
    BUFFER_SIZE = 1024

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 

    #Connect to the port and listen based on command line arguments
    if sys.argv[3] == "-l":
        #print(sys.argv[4])
        server.bind((TCP_IP, int(sys.argv[4])))
    if sys.argv[3] == "-g":
        #print(sys.argv[5])
        server.bind((TCP_IP, int(sys.argv[5]))) 
    print("Waiting for Connection")

    server.listen(1) #maximum of one connection
    return server

#This function receives the messages and returns them.  It takes
#one parameter, the connection where it will receive a message from.
def receiveMessage(conn):
    message = conn.recv(2048)
    return message

#Program starts here
startup()
server = dataConnection()
newUser = 0
if newUser == 0: 
    conn, addr = server.accept() 

    print("Server Connected")
    newUser = 1
#create placeholder values
message = 1
fileName = "Placeholder"
#if the user requested the directory
if sys.argv[3] == "-l":
    print ("*Start of directory*")
    while (message != "*End of directory*"):
        message = receiveMessage(conn)
        print (message)
#if the user requested a file
if sys.argv[3] == "-g":
    message = conn.recv(8)
    #if message == "Filename":
    #print("First Message")
    #print(message)
    #conn.send("2")
    #while message != "quit": 

    if 1 == 1:
        #print (message)
        if message == "":
            print("Connection Closed by Client")
            #break
        if message == "Filename":

            #print("Filename received")

            fileName = receiveMessage(conn)
            #print(fileName)
            if fileName != "noFILE":
                
                #print("Filename is:")
                #print(fileName)
                index = fileName.index(".")
                fileName = fileName[0:index]
                fileName = fileName + " - (Copy).txt"
                #print (fileName)

                file = open(fileName, "a")
                #print(message)
                while message != "End of the file":# or message != "check file" or message !="":
                    message = receiveMessage(conn)
                    #print(message)
                    if message != "End of the file":# or message != "check file":
                        file.write(message)
                file.close()
                print("File transfer complete")
                if message == "check file":
                    #os.remove(fileName)
                    print("Error, file does not exist.")
            else:
                print("Error, file does not exist.")

        #message = sendMessage(conn, message, quit)

conn.close()
server.close() 
exit()