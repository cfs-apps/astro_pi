"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    Purpose:
      Provide a command and telemetry interface between the Astro Pi
      cFS app and the Raspberry Pi python Sense HAT interface.
    
    Notes:
      1. The goal of this script in combination with the Astro Pi cFS apps is
         to provide enough functionality that allows students to perform the 
         ESA's Astro Pi exercises https://astro-pi.org/
      2. This script requires the Sense HAT package. It should be part of
         the Raspian OS. You can manually install the package as follows: 
         sudo apt-get install sense-hat
      3. The Astro Pi cFS app's command and telemetry interfaces are defined
         in astro_pi.xml

"""

import configparser
import socket
import threading
import time
import json

from sense_hat import SenseHat
sense = SenseHat()

RUN_SCRIPT_TEXT_CMD = 1
RUN_SCRIPT_FILE_CMD = 2

JMSG_PREFIX = 'basecamp/script:'
TEST1_JSON  = "{\"command\": 1, \"script-file\": \"Undefined\", \"script-text\": \"print('Hello world')\"}"
TEST2_JSON  = "{\"command\": 1, \"script-file\": \"Undefined\", \"script-text\": \"from sense_hat import SenseHat\\nsense = SenseHat()\\nsense.show_message('Hello world')\"}"
TEST1_JMSG  = JMSG_PREFIX + TEST1_JSON
TEST2_JMSG  = JMSG_PREFIX + TEST2_JSON

config = configparser.ConfigParser()
config.read('astro_pi.ini')
RX_LOOP_DELAY = config.getint('APP','RX_LOOP_DELAY')
TX_LOOP_DELAY = config.getint('APP','TX_LOOP_DELAY')

JMSG_MAX_LEN = config.getint('JMSG','JMSG_MAX_LEN')
JMSG_TOPIC_SCRIPT_CMD_NAME = config.get('JMSG','JMSG_TOPIC_SCRIPT_CMD_NAME')
JMSG_TOPIC_CSV_TLM_NAME    = config.get('JMSG','JMSG_TOPIC_CSV_TLM_NAME')

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
CFS_IP_ADDR  = config.get('NETWORK','CFS_IP_ADDR')
CFS_APP_PORT = config.getint('NETWORK','CFS_APP_PORT')
PY_APP_PORT  = config.getint('NETWORK','PY_APP_PORT')



def tx_thread():
    
    i = 1
    while True:
        payload = read_tlm_parameters()
        jmsg = JMSG_TOPIC_CSV_TLM_NAME + '{"name": "RPI-0", "seq-count": %d, "date-time": "00/00/0000 00:00:00",  "parameters": "%s"}' % (i,payload)
        print(f'>>> Sending message {jmsg}')
        sock.sendto(jmsg.encode('ASCII'), (CFS_IP_ADDR, CFS_APP_PORT))
        time.sleep(TX_LOOP_DELAY)
        i += 1

        
def rx_thread():

    rx_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    rx_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    rx_socket.bind((CFS_IP_ADDR, PY_APP_PORT))
    rx_socket.setblocking(False)
    rx_socket.settimeout(1000)

    while True:
        jmsg = None
        print("Pending for recvfrom")
        try:
            while True:
                datagram, host = rx_socket.recvfrom(JMSG_MAX_LEN)
                if datagram:
                    jmsg = datagram
                if jmsg:
                    jmsg_str = jmsg.decode('utf-8')
                    print(f'*****\nReceived from {host} JMSG {len(jmsg_str)}: {jmsg_str}\n')
                    process_jmsg_cmd(jmsg_str.replace("\x00", "").replace("\x01", ""))
        except socket.timeout:
            pass
        print('*****\n\n')
        time.sleep(RX_LOOP_DELAY)


def process_jmsg_cmd(jmsg_str):

    try:
        # Text following prefix is assumed to be JSON message 
        if jmsg_str.startswith(JMSG_TOPIC_SCRIPT_CMD_NAME):
            json_str = jmsg_str.replace(JMSG_TOPIC_SCRIPT_CMD_NAME, "")
            print(f'>>json {len(json_str)}: {json_str}\n')
            json_str2 = json_str.replace('\n','\\n')
            print(f'>>json2: {json_str2}\n')
            json_dict = json.loads(json_str2)
            command = json_dict["command"]
            if command == RUN_SCRIPT_TEXT_CMD:
                print(f'>>json_dict: {json_dict}\n')
                exec(json_dict["script-text"])
                print("")                
            elif command == RUN_SCRIPT_FILE_CMD:
                exec(open(json_dict["script-file"]).read())
            else:
                print(f'Received JMSG with invalid command {command}')
        else:
            print(f'Received JMSG not addresssed to Astro Pi. Expected {JMSG_TOPIC_NAME_PREFIX}')
    except Exception as e:
        print(f'Astro Pi JMSG processing exception: {e}\n')


def read_tlm_parameters():
    """
    Gain is simply the sensitivity of the sensor. It can have values of 1, 4, 16 or 60.
    Integration cycles are the time that the the sensor takes between measuring the light. 
    Each integration cycle is 2.4 milliseconds long, and the number of integration cycles can be any number between 1 and 256.
    """
    
    sense.clear()

    orientation = sense.get_orientation()
    roll  = orientation["roll"]
    pitch = orientation["pitch"]
    yaw   = orientation["yaw"]

    acceleration = sense.get_accelerometer_raw()
    accel_x = acceleration['x']
    accel_y = acceleration['y']
    accel_z = acceleration['z']

    x=round(accel_x, 0)
    y=round(accel_y, 0)
    z=round(accel_z, 0)
    
    pressure    = sense.get_pressure()
    temperature = sense.get_temperature()
    humidity    = sense.get_humidity()
    
    red, green, blue, clear = sense.colour.colour

    parameters = f"rate-x,{roll},rate-y,{pitch},rate-z,{yaw},accel-x,{accel_x},accel-y,{accel_y},accel-z,{accel_z},pressure,{pressure},temperature,{temperature},humidity,{humidity},red,{red},green,{green},blue,{blue},clear,{clear}"

    return payload

          
if __name__ == "__main__":

    rx = threading.Thread(target=rx_thread)
    rx.start()
   
    tx = threading.Thread(target=tx_thread)
    tx.start()
 
    #process_jmsg(TEST1_JMSG)
    #process_jmsg(TEST2_JMSG)