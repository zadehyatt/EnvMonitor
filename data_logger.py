#!/usr/bin/env python3
"""
Environmental Monitor Data Logger
Reads sensor data from STM32 over serial and logs to CSV file
"""

import serial
import serial.tools.list_ports
import csv
import datetime
import time
import sys

def find_stm32_port():
    """Find the STM32 COM port automatically"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for ST-Link or STM devices
        if 'STM' in port.description or 'ST-Link' in port.description:
            return port.device
    return None

def main():
    print("=== Environmental Monitor Data Logger ===\n")
    
    # Find STM32 port
    port = find_stm32_port()
    
    if port is None:
        print("ERROR: Could not find STM32 device!")
        print("\nAvailable COM ports:")
        for p in serial.tools.list_ports.comports():
            print(f"  {p.device}: {p.description}")
        print("\nPlease specify the COM port manually:")
        port = input("Enter COM port (e.g., COM3): ").strip()
    
    print(f"Using port: {port}")
    
    # Create filename with timestamp
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"envmonitor_log_{timestamp}.csv"
    print(f"Logging to: {filename}\n")
    
    try:
        # Open serial port
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for connection to stabilize
        
        # Open CSV file
        with open(filename, 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile)
            
            print("Connected! Logging data... (Press Ctrl+C to stop)\n")
            print("Time(ms)  | Temp(C) | Humidity(%) | Pressure(hPa) | Feels Like(C)")
            print("-" * 75)
            
            line_count = 0
            
            while True:
                # Read line from serial
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if line:
                    # Write to CSV
                    csvwriter.writerow([line])
                    csvfile.flush()  # Make sure data is written immediately
                    
                    # Display on console
                    if ',' in line:
                        parts = line.split(',')
                        if len(parts) == 5:
                            try:
                                time_ms = int(parts[0])
                                temp = float(parts[1])
                                humidity = float(parts[2])
                                pressure = float(parts[3])
                                feels = float(parts[4])
                                
                                print(f"{time_ms:8d}  | {temp:6.2f}  | {humidity:11.2f} | {pressure:13.2f} | {feels:13.2f}")
                                line_count += 1
                                
                                # Print update every 10 lines
                                if line_count % 10 == 0:
                                    print(f"  [{line_count} readings logged]")
                            except ValueError:
                                # Header line or malformed data
                                if line_count == 0:
                                    print(f"Header: {line}")
                    else:
                        print(line)  # Print any status messages
    
    except serial.SerialException as e:
        print(f"\nERROR: Could not open serial port {port}")
        print(f"Details: {e}")
        sys.exit(1)
    
    except KeyboardInterrupt:
        print(f"\n\nLogging stopped.")
        print(f"Total readings logged: {line_count}")
        print(f"Data saved to: {filename}")
        sys.exit(0)
    
    except Exception as e:
        print(f"\nERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
