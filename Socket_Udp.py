import socket

localIP     = "ip-172-31-31-193.ec2.internal"
localPort   = 5050
bufferSize  = 1024

msgFromServer       = "Conectado"
bytesToSend         = str.encode(msgFromServer)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))

print("Servidor UDP-Proyecto Panel Solar")

# Listen for incoming datagrams
while(True):
    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
    message = bytesAddressPair[0]
    address = bytesAddressPair[1]
    clientMsg = "Mensaje de Cliente:{}".format(message)
    clientIP  = "IP de cliente:{}".format(address)
    print(clientMsg)
    print(clientIP)

    # Sending a reply to client
    UDPServerSocket.sendto(bytesToSend, address)