# Silent Server
Silent is an extremely lightweight, high quality - low latency voice chat made for gaming.<br>
Must be used with the Silent (client) application.<br> 
<br>
Made with Qt Framework, Winsock2 and Windows Multimedia System.

# How to start your Silent Server
1. Install Silent Server from the releases tab.<br>
2. Open your router settings:<br>
&nbsp;&nbsp;&nbsp;&nbsp;2. 1. Press (Win + R) ('Run' window will be opened), type 'cmd' and press Enter.<br>
&nbsp;&nbsp;&nbsp;&nbsp;2. 2. Type 'ipconfig' and press Enter.<br>
&nbsp;&nbsp;&nbsp;&nbsp;2. 3. Find a line which says something like 'Default Gateway . . . . . . . . . : 192.168.1.1' (note that the address can be different on your PC).<br>
&nbsp;&nbsp;&nbsp;&nbsp;2. 4. Copy this address and paste it into your browser URL input section.<br>
&nbsp;&nbsp;&nbsp;&nbsp;2. 5. Login into your router settings (usually by entering 'Admin' to both 'Username' and 'Password' fields).<br>
3. Forward TCP port 51337 (text chat) and UDP port 51337 (voice chat):<br>
&nbsp;&nbsp;&nbsp;&nbsp;3. 1. This differs on every router but you need to find a page with a name like 'Virtual Servers' or 'Port Forwarding'. Try googling how to forward a port in your router model.<br>
&nbsp;&nbsp;&nbsp;&nbsp;3. 2. On the 'port forward' page add new preset and set next params:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3. 2. 1. 'Protocol' should be 'TCP', 'UDP' or 'Both'. Note that if you can only choose 'TCP' or 'UDP' you will need to do the same thing but for other protocol.<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3. 2. 2. 'External Port' should be '51337'. Note that later (in the Silent Server settings) you can change the default 51337 port to any other port you want and so you will need to change it also here.<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3. 2. 3. 'Internal Port' should also be '51337'.<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3. 2. 4. 'Internal Client' should be equal to the 'IPv4 Address' in the console (when we typed 'ipconfig').<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;3. 2. 5. Apply this new preset. Note that your local IPv4 address may change every time you turn on your router so I recommend you to google 'how to set static local ip address on windows'.<br>
4. Open Silent Server, click Chat -> Start Server.<br>
&nbsp;&nbsp;&nbsp;&nbsp;4. 1. Also, don't forget to check Chat -> Settings menu.<br>
5. Tell your friends your public IPv4 address.<br>
&nbsp;&nbsp;&nbsp;&nbsp;5. 1. You can find your public IPv4 address by googling something like 'what's my ip address'.<br>

# Build
Silent is built with the MSVC 2017 64 bit compiler and Qt Framework (through Qt Creator).