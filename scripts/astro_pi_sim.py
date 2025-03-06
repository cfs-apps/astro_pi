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
      Provide an environment to test basic functionality of the 
      Astro Pi cFS app. It processes command scripts and generates
      ismulated telemetry. 
    
    Notes:
      1. Commands scripts that required the Sense HAT hardware will 
         generate exceptions.

"""

import configparser
import socket
import threading
import time
import json

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
JMSG_TOPIC_CSV_TLM_NAME = config.get('JMSG','JMSG_TOPIC_CSV_TLM_NAME')

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
CFS_IP_ADDR  = config.get('NETWORK','CFS_IP_ADDR')
CFS_APP_PORT = config.getint('NETWORK','CFS_APP_PORT')
PY_APP_PORT  = config.getint('NETWORK','PY_APP_PORT')



def tx_thread():
    
    i = 1
    while True:
        cont = input ("Enter to send")
        jmsg = JMSG_TOPIC_CSV_TLM_NAME + '{"name": "null", "seq-count": 0, "date-time": "00/00/0000 00:00:00",  "parameters": "rate-x,1.0,rate-y,2.0,rate-z,3.0,accel-x,4.0,accel-y,5.0,accel-z,6.0,pressure,7.0,temperature,8.0,humidity,9.0,red,1,green,2,blue,3,clear,4"}'
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
                    process_jmsg(jmsg_str.replace("\x00", "").replace("\x01", ""))
        except socket.timeout:
            pass
        print('*****\n\n')
        time.sleep(RX_LOOP_DELAY)


def process_jmsg(jmsg_str):

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
            print(f'Received JMSG not addressed to Astro Pi. Expected {JMSG_TOPIC_SCRIPT_CMD_NAME}')
    except Exception as e:
        print(f'Astro Pi JMSG processing exception: {e}\n')

if __name__ == "__main__":

    rx = threading.Thread(target=rx_thread)
    rx.start()
   
    tx = threading.Thread(target=tx_thread)
    tx.start()
 
    #process_jmsg(TEST1_JMSG)
    #process_jmsg(TEST2_JMSG)