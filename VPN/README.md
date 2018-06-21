## VPN

### Details :
- This is a VPN program entirely wriiten in C using linux networking libraries.
- It is based on **TUNNELING** protocol.
- For running this program you need to have read/write permissions on **"dev/net/tun"**.
- Only for UNIX.

### How to use :
- You need to have root access ``` sudo -su root ```.
- complie the vpn.c file ``` gcc vpn.c -o vpn```.
- Then establish a server ``` ./vpn -i tun0 -s -p 5555 -u ```
- Configure the tun/tap interface you have created by opening a new terminal.
- In new terminal write ```sudo ip addr add 10.0.0.2/24 dev tun0```
- ``` sudo ip link set tun0 up```
- ``` sudo ifconfig tun0 up ```
- Then make a client and connect to server ``` ./vpn -i tun1 -c 127.0.0.1 -p 5555 -u ```.
- Configure client ``` sudo ip addr add 10.0.5.2/8 dev tun1``` then ``` sudo ip link set tun1 up ```
- You can test you connection using ping ```ping 10.0.0.2```

### Points to notice :
- Root access is need for running all commands.
- The name "tun0" or "tun1" are just examples any non empty string will work.
- The connection is made on local host . You can make it in two different machines.
- for more flags and good understanding  write ```./vpn -h```.

### Aim :
- To understand the basic working of VPN's.
- To understand virtual interfaces like tun/tap.

### Future additions :
- The tunnel created so far are not secure.
- I will use openSSL libraries to encrypt and decrypt data and also for user authentication.

**Hope you like it ;)** 
