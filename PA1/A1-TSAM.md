
1. **Always check the cables**
![[Screenshot from 2026-08-25 17-58-38.png]]
(b)
	- 5. 
![[Screenshot from 2026-08-25 19-05-27.png]]
(c)  **In your own words, explain what each of the three commands does. Hint: it may help to read the manual (man page) for ncat and tcpdump**.

sudo tcpdump −vAX −i ens192 ’port 5000’:

Here we have a tcpdump command, used to capture and display network traffic on an interface, run with sudo since capturing raw traffic needs elevated privileges. It's paired with the Options -v for slightly more verbose output (things like TTL, packet length and checksum verification), -A which prints each packet's data in ASCII, and -X which additionally prints it in hex, both handy for reading the actual contents of packets. -i ens192 tells it which interface to listen on, and 'port 5000' filters it down to only traffic on that port, the same one the ncat commands above were using.

ncat −ln 5000:

here we have a ncat command which alone is a command used to read and write data across networks but here it is also paired with the Options -l which makes it listen for connections instead of connecting to a remote machine which is the default and -n which disables hostname resolution so it skips resolving names to IPs and vice versa rather than requiring one.

ncat 130.208.246.98 5000

Here the ncat command without any Options so it has the Hostname which was disabled in the last command and the port aswell which is the default way of using Ncat, so this particular command connects to whatever is listening on that IP and port and lets you exchange data with it interactively

(e) **How does the tcpdump output differ when you used UDP vs TCP? Why?**

![[TCP vs UDP]]

