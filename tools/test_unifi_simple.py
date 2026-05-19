#!/usr/bin/env python3
"""Test UniFi API - Simple version"""

import os
import re
import requests
import json
from requests.packages import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


def read_credentials():
    """Read credentials from include/credentials.h."""
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'include', 'credentials.h'), 'r') as f:
        content = f.read()
    return {
        'host': re.search(r'#define UNIFI_HOST "([^"]*)"', content).group(1),
        'username': re.search(r'#define UNIFI_USERNAME "([^"]*)"', content).group(1),
        'password': re.search(r'#define UNIFI_PASSWORD "([^"]*)"', content).group(1),
    }


_creds = read_credentials()
UNIFI_HOST = _creds['host']
UNIFI_USERNAME = _creds['username']
UNIFI_PASSWORD = _creds['password']
BASE_URL = f"https://{UNIFI_HOST}"
session = requests.Session()
session.verify = False

print("=== UniFi API Test ===")
print(f"Host: {UNIFI_HOST}")

# Login
print("\n1. Login...")
login_data = {"username": UNIFI_USERNAME, "password": UNIFI_PASSWORD, "remember": False}
response = session.post(f"{BASE_URL}/api/auth/login", json=login_data)
print(f"   Status: {response.status_code}")

if response.status_code != 200:
    print(f"   Login failed: {response.text}")
    exit(1)

print("   Login successful!")

# Test endpoints
endpoints = [
    ("/proxy/network/api/s/default/stat/sta", "Clients"),
    ("/proxy/network/api/s/default/stat/device", "Devices"),
    ("/proxy/network/api/s/default/stat/health", "Health"),
    ("/proxy/network/api/s/default/stat/sysinfo", "Sysinfo"),
]

print("\n2. Testing endpoints:")
for endpoint, name in endpoints:
    print(f"\n{name}: {endpoint}")
    response = session.get(f"{BASE_URL}{endpoint}")
    print(f"   Status: {response.status_code}")
    
    if response.status_code == 200:
        data = response.json()
        if data.get("meta", {}).get("rc") == "ok":
            result = data.get("data", [])
            print(f"   Found {len(result)} items")
            
            # Show specific data
            if name == "Clients" and result:
                print(f"   Connected clients: {len(result)}")
                for client in result[:3]:
                    print(f"     - {client.get('hostname', 'Unknown')} ({client.get('ip', 'N/A')})")
                    
            elif name == "Devices" and result:
                for device in result:
                    dtype = device.get('type', '')
                    dname = device.get('name', 'Unknown')
                    print(f"     - {dname} (type: {dtype})")
                    
                    # Look for gateway device
                    if dtype in ['ugw', 'udm', 'usg']:
                        print(f"       Found gateway: {dname}")
                        
                        # Check uplink
                        if 'uplink' in device:
                            uplink = device['uplink']
                            print(f"       Uplink speed: {uplink.get('speed', 0)} Mbps")
                            print(f"       RX bytes: {uplink.get('rx_bytes', 0)}")
                            print(f"       TX bytes: {uplink.get('tx_bytes', 0)}")
                        
                        # Check port_table
                        if 'port_table' in device:
                            for port in device['port_table']:
                                if port.get('name') == 'wan':
                                    print(f"       WAN port found:")
                                    print(f"         RX rate: {port.get('rx_rate', 0)} bps")
                                    print(f"         TX rate: {port.get('tx_rate', 0)} bps")
                                    print(f"         RX bytes: {port.get('rx_bytes', 0)}")
                                    print(f"         TX bytes: {port.get('tx_bytes', 0)}")
                        
                        # Check stat
                        if 'stat' in device:
                            stat = device['stat']
                            if 'gw' in stat:
                                gw = stat['gw']
                                print(f"       Gateway stats:")
                                print(f"         WAN RX: {gw.get('wan-rx_bytes', 0)} bytes")
                                print(f"         WAN TX: {gw.get('wan-tx_bytes', 0)} bytes")
                                
            elif name == "Health" and result:
                for item in result:
                    subsystem = item.get('subsystem', '')
                    print(f"     - {subsystem}: {item.get('status', 'unknown')}")
                    
                    if subsystem == 'wan':
                        print(f"       rx_bytes-r: {item.get('rx_bytes-r', 0)}")
                        print(f"       tx_bytes-r: {item.get('tx_bytes-r', 0)}")
                        
                    elif subsystem == 'wlan':
                        print(f"       num_sta: {item.get('num_sta', 0)}")
                        
            elif name == "Sysinfo" and result:
                info = result[0] if result else {}
                print(f"     - Uptime: {info.get('uptime', 0)} seconds")
                print(f"     - WAN IP: {info.get('wan_ip', 'N/A')}")

print("\n3. Checking device-basic endpoint...")
response = session.get(f"{BASE_URL}/proxy/network/api/s/default/rest/device")
if response.status_code == 200:
    data = response.json()
    devices = data.get('data', [])
    print(f"   Found {len(devices)} devices")
    
    for device in devices:
        if device.get('type') in ['ugw', 'udm']:
            print(f"\n   Gateway: {device.get('name')}")
            print(f"   Model: {device.get('model')}")
            
            # Check for real-time stats
            if 'uptime' in device:
                print(f"   Uptime: {device['uptime']} seconds")
            
            if 'wan1' in device:
                wan = device['wan1']
                print(f"   WAN1 stats:")
                print(f"     IP: {wan.get('ip', 'N/A')}")
                
print("\n=== Test Complete ===")