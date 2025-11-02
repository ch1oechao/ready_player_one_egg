#!/usr/bin/env python3
"""
Serial Port Multiplexer - Allows multiple applications to access the same serial port
Usage: python3 serial_multiplexer.py /dev/cu.wchusbserial210 115200
Then connect applications to localhost:5000, localhost:5001, etc.
"""

import serial
import socket
import threading
import time
import sys

class SerialMultiplexer:
    def __init__(self, serial_port, baud_rate, num_tcp_ports=3):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.num_tcp_ports = num_tcp_ports
        self.base_port = 5000
        
        # Initialize serial connection
        try:
            self.ser = serial.Serial(serial_port, baud_rate, timeout=1)
            print(f"✓ Connected to {serial_port} at {baud_rate} baud")
        except Exception as e:
            print(f"✗ Failed to connect to serial port: {e}")
            sys.exit(1)
        
        # Store TCP connections
        self.tcp_connections = []
        self.tcp_servers = []
        
        # Start TCP servers
        for i in range(num_tcp_ports):
            port = self.base_port + i
            self.start_tcp_server(port)
    
    def start_tcp_server(self, port):
        """Start a TCP server that mirrors the serial connection"""
        try:
            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(('localhost', port))
            server.listen(5)
            self.tcp_servers.append(server)
            
            print(f"✓ TCP server listening on localhost:{port}")
            
            # Start accepting connections in a separate thread
            thread = threading.Thread(target=self.accept_connections, args=(server, port))
            thread.daemon = True
            thread.start()
            
        except Exception as e:
            print(f"✗ Failed to start TCP server on port {port}: {e}")
    
    def accept_connections(self, server, port):
        """Accept TCP connections and add them to the multiplexer"""
        while True:
            try:
                client, addr = server.accept()
                print(f"✓ Client connected to port {port} from {addr}")
                
                # Add to connections list
                self.tcp_connections.append({
                    'socket': client,
                    'port': port,
                    'addr': addr
                })
                
                # Start handling this client
                thread = threading.Thread(target=self.handle_tcp_client, args=(client, port))
                thread.daemon = True
                thread.start()
                
            except Exception as e:
                print(f"Error accepting connection on port {port}: {e}")
                break
    
    def handle_tcp_client(self, client, port):
        """Handle data from TCP client to serial"""
        try:
            while True:
                data = client.recv(1024)
                if not data:
                    break
                
                # Send to serial port
                self.ser.write(data)
                print(f"TCP→Serial (port {port}): {data.decode('utf-8', errors='ignore').strip()}")
                
        except Exception as e:
            print(f"TCP client on port {port} disconnected: {e}")
        finally:
            # Remove from connections
            self.tcp_connections = [conn for conn in self.tcp_connections if conn['socket'] != client]
            client.close()
    
    def serial_to_tcp_forwarder(self):
        """Forward data from serial to all TCP connections"""
        while True:
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.readline()
                    if data:
                        print(f"Serial→TCP: {data.decode('utf-8', errors='ignore').strip()}")
                        
                        # Send to all connected TCP clients
                        disconnected = []
                        for conn in self.tcp_connections:
                            try:
                                conn['socket'].send(data)
                            except:
                                disconnected.append(conn)
                        
                        # Remove disconnected clients
                        for conn in disconnected:
                            self.tcp_connections.remove(conn)
                            conn['socket'].close()
                
                time.sleep(0.01)  # Small delay to prevent CPU spinning
                
            except Exception as e:
                print(f"Error in serial reader: {e}")
                break
    
    def run(self):
        """Start the multiplexer"""
        print(f"\n🚀 Serial Multiplexer running!")
        print(f"📡 Serial: {self.serial_port} @ {self.baud_rate} baud")
        print(f"🌐 TCP ports: {self.base_port}-{self.base_port + self.num_tcp_ports - 1}")
        print(f"\nConnect your applications to:")
        for i in range(self.num_tcp_ports):
            print(f"  • localhost:{self.base_port + i}")
        print(f"\nPress Ctrl+C to stop\n")
        
        # Start serial to TCP forwarder
        serial_thread = threading.Thread(target=self.serial_to_tcp_forwarder)
        serial_thread.daemon = True
        serial_thread.start()
        
        try:
            # Keep main thread alive
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\n🛑 Shutting down multiplexer...")
            self.cleanup()
    
    def cleanup(self):
        """Clean up resources"""
        # Close all TCP connections
        for conn in self.tcp_connections:
            conn['socket'].close()
        
        # Close TCP servers
        for server in self.tcp_servers:
            server.close()
        
        # Close serial connection
        if self.ser.is_open:
            self.ser.close()
        
        print("✓ Cleanup complete")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 serial_multiplexer.py <serial_port> <baud_rate>")
        print("Example: python3 serial_multiplexer.py /dev/cu.wchusbserial210 115200")
        sys.exit(1)
    
    serial_port = sys.argv[1]
    baud_rate = int(sys.argv[2])
    
    multiplexer = SerialMultiplexer(serial_port, baud_rate)
    multiplexer.run()