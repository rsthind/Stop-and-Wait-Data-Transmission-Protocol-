# Stop-and-Wait-Data-Transmission-Protocol-
</br>

In this project I implemented stop-and-wait protocol with acknowledge and negative acknowledgements as well as retransmissions on the negative acknowledgements. This was implemented with construction and destruction of packets and a thread safe message queue.


## Testing
</br>

$ make </br>
</br>

Run the server: </br>
</br>
$ python rtp-server -p [port number] [-c corruption_rate] </br>

Run a client to send a message to server: </br>
</br>
$ ./rtp-client 127.0.0.1 8080