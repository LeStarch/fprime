"""
GDS Interface:

Interfaces for interacting with the F prime GDS layer. This interfaces deals in full FSW packets, so it it is left
up to the user to handle transmission correctness, checksum calculation, etc.

@author lestarch
"""
from __future__ import print_function
import sys
import struct
import socket
import functools
import threading


class TCPGDSInterface(object):
    """
    Interface class defining necessary functions to talk to the GDS.
    """
    def __init__(self, address="127.0.0.1", port=50000, timeout=0.200):
        """
        Initialize this interface with the address and port needed to connect to the GDS.
        :param address: Address of the tcp server. Default 127.0.0.1
        :param port: port of the tcp server. Default: 50000
        """
        # Socket variables
        self.address = address
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(timeout)
        self.timeout = timeout
        self.recv_lock = threading.Lock()
        self.send_lock = threading.Lock()

        # Callback and threading variables
        self.callbacks = []
        self.thread = threading.Thread(target=self.run, args=())
        self.running = False
        self.connected = False

    def open(self):
        """
        Opens the connection to the TCP server. Raises exception on connection issues.
        """
        if self.connected:
            return
        self.socket.connect((self.address, self.port))
        self.connected = True
        # Kick off thread, if registered
        if not self.running and self.callbacks:
            self.running = True
            self.thread.start()
        # Opening volley
        self.socket.sendall(b"Register FSW\n")

    def close(self):
        """
        Close the connection to the GDS TCP server.
        """
        if not self.connected:
            return
        if self.running:
            self.running = False
            self.thread.join()
            self.socket.settimeout(self.timeout)
        self.socket.shutdown(socket.SHUT_RDWR)

    def register(self, callback):
        """
        Registers a callback used to supply data read from the socket back to the caller.
        :param callback: callback to call when data is read
        """
        self.callbacks.append(callback)
        # Kick off thread, if registered
        if not self.running and self.connected:
            self.running = True
            self.thread.start()

    def run(self):
        """
        Run function supplied to the thread. This will run until it is asked to stop by setting
        self.running to False
        """
        # First make it a blocking socket
        self.socket.settimeout(None)
        while self.running and self.connected:
            data = self.poll()
            if data is not None:
                for callback in self.callbacks:
                    callback(data)
        self.running = False

    def write(self, packet):
        """
        Write a packet out to the tcp socket server
        :param packet: bytes object of data to write out to the socket server
        """
        data = b"A5A5 GUI %s" % (packet)
        with self.send_lock:
            self.socket.sendall(data)

    def poll(self):
        """
        Polls the interface looking for a packet of data.
        :return: bytes object containing packet
        """
        try:
            with self.recv_lock:
                return self.socket.recv(4096)
        except socket.timeout:
            return None


if __name__ == "__main__":
    polling = (len(sys.argv) != 2 or sys.argv[1].lower() != "callback")
    gds = TCPGDSInterface()
    gds.open()
    try:
        # While polling, loop and read data
        while polling:
            data = gds.poll()
            if data is not None:
                print("Data Received:", data)
        if not polling:
            gds.register(functools.partial(print, "Data Called:"))
    except KeyboardInterrupt:
        gds.close()
