## Port Scanner 

### Details  :

- This is a port scanner entirely written in C language with the use of linux networking libraries.
- It includes three types of scan **TCP CONNECT** , **TCP SYN** , **UDP SCAN**.
- **TCP CONNECT SCAN** : Making a three way handshake on specified ports.
- **TCP SYN SCAN** : Sending a **SYN** Packet to the target ip on a specific port .Then wating for a response and check if the tcp header has both  **SYN AND ACK** Packet. Also it is necessary to check if source ip adress of response packet is same as destination ip of sent syn packet.
- **UDP SCAN** : Send UDP packets to target ip and wait for some time if **ICMP** packet reaches then port is closed.

### How to use :
- Compile the C file  ```gcc PortScanner.c -o scan```.
- For TCP CONNECT SCAN  -  ```./scan < target ip > tcp <portLow> <portHigh>```.
- For TCP SYN SCAN  -  ```./scan < target ip > stcp <portLow> <portHigh>```.
- For UDP SCAN  -  ```./scan < target ip > udp <portLow> <portHigh>```.

- portLow and portHigh are range of ports you want to scan.

### Aim of project :
- To get started with UNIX Networking.
- Understanding the working of sockets at a much lower level.
- Understanding networking at basic level.

### Refrences :
- UNIX network Programming by W.Richard Stevens Volume 1.

**Note** : For UDP and TCP SYN Scanning you need to have root access. No worry if not the program with inform you about this via a prompt.

