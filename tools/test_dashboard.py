#!/usr/bin/env python3
"""Test UniFi dashboard endpoint for accurate data"""

import os
import re
import requests
import json
from datetime import datetime, timedelta
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

print("=== Testing UniFi Dashboard Data ===")

# Login
login_data = {"username": UNIFI_USERNAME, "password": UNIFI_PASSWORD, "remember": False}
response = session.post(f"{BASE_URL}/api/auth/login", json=login_data)
print(f"Login status: {response.status_code}")

if response.status_code == 200:
    print("\nTesting different endpoints for accurate data:")
    
    # Test dashboard endpoint
    print("\n1. Dashboard stats:")
    dash = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/dashboard")
    if dash.status_code == 200:
        data = dash.json().get('data', [])
        if data:
            print(f"   Dashboard data available")
            for item in data:
                if isinstance(item, dict):
                    for key in ['num_sta', 'num_user', 'num_guest', 'num_iot', 
                               'wan_ip', 'latency', 'uptime', 'mem', 'cpu']:
                        if key in item:
                            print(f"   {key}: {item[key]}")
    
    # Test device-basic for total count
    print("\n2. Device count from device-basic:")
    basic = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/device-basic")
    if basic.status_code == 200:
        data = basic.json().get('data', [])
        print(f"   Network devices: {len(data)}")
    
    # Test sta endpoint for client count
    print("\n3. Client count from sta:")
    sta = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/sta")
    if sta.status_code == 200:
        data = sta.json().get('data', [])
        print(f"   Connected clients: {len(data)}")
    
    # Test alluser endpoint
    print("\n4. All users from alluser:")
    users = session.get(f"{BASE_URL}/proxy/network/api/s/default/rest/user")
    if users.status_code == 200:
        data = users.json().get('data', [])
        # Count only active users
        active = [u for u in data if u.get('_is_guest_by_uap', False) or u.get('is_wired', False) or u.get('_uptime_by_uap')]
        print(f"   Total users in database: {len(data)}")
        print(f"   Active/recent users: {len(active)}")
        
    # Test health for latency
    print("\n5. WAN latency from health:")
    health = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/health")
    if health.status_code == 200:
        data = health.json().get('data', [])
        for item in data:
            if item.get('subsystem') == 'wan':
                print(f"   WAN Status: {item.get('status')}")
                print(f"   WAN Latency: {item.get('latency', 'N/A')} ms")
                print(f"   WAN IP: {item.get('wan_ip', 'N/A')}")
                if 'uptime_stats' in item:
                    print(f"   Uptime stats: {item['uptime_stats']}")
                    
    # Test sites for overview
    print("\n6. Site overview:")
    sites = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/sites")
    if sites.status_code == 200:
        data = sites.json().get('data', [])
        if data:
            site = data[0]
            print(f"   Site name: {site.get('desc', 'N/A')}")
            for key in ['num_sta', 'num_sw', 'num_ap', 'num_adopted', 'lan_ip', 'wan_ip']:
                if key in site:
                    print(f"   {key}: {site[key]}")
                    
    # Test for monthly data
    print("\n7. Monthly data usage:")
    # Get current month timestamps
    now = datetime.now()
    start_of_month = datetime(now.year, now.month, 1)
    start_ms = int(start_of_month.timestamp() * 1000)
    end_ms = int(now.timestamp() * 1000)
    
    # Try monthly report
    monthly_url = f"{BASE_URL}/proxy/network/api/s/default/stat/report/monthly.site"
    monthly = session.get(monthly_url)
    if monthly.status_code == 200:
        data = monthly.json().get('data', [])
        print(f"   Monthly reports: {len(data)} entries")
        total_bytes = 0
        for entry in data:
            if 'wan-rx_bytes' in entry:
                total_bytes += entry['wan-rx_bytes']
            if 'wan-tx_bytes' in entry:
                total_bytes += entry['wan-tx_bytes']
        print(f"   Total monthly bytes: {total_bytes / (1024**3):.2f} GB")
    
    # Try 5-minute stats for current month
    print("\n8. Calculate monthly from 5-minute stats:")
    five_min_url = f"{BASE_URL}/proxy/network/api/s/default/stat/report/5minutes.site"
    params = {'attrs': ['wan-tx_bytes', 'wan-rx_bytes'], 'start': start_ms, 'end': end_ms}
    five_min = session.get(five_min_url, params=params)
    if five_min.status_code == 200:
        data = five_min.json().get('data', [])
        print(f"   5-minute entries: {len(data)}")
        total_rx = sum(e.get('wan-rx_bytes', 0) for e in data)
        total_tx = sum(e.get('wan-tx_bytes', 0) for e in data)
        total_gb = (total_rx + total_tx) / (1024**3)
        print(f"   Total RX: {total_rx / (1024**3):.2f} GB")
        print(f"   Total TX: {total_tx / (1024**3):.2f} GB")
        print(f"   Total this month: {total_gb:.2f} GB")
    
    # Get system info for uptime
    print("\n9. System uptime:")
    sysinfo = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/sysinfo")
    if sysinfo.status_code == 200:
        data = sysinfo.json().get('data', [])
        if data:
            info = data[0]
            uptime_sec = info.get('uptime', 0)
            uptime_days = uptime_sec / 86400
            # Calculate uptime percentage (100% = no downtime this month)
            month_seconds = (now - start_of_month).total_seconds()
            uptime_percent = min(100, (uptime_sec / month_seconds) * 100) if month_seconds > 0 else 0
            print(f"   Uptime: {uptime_sec} seconds")
            print(f"   Uptime days: {uptime_days:.1f}")
            print(f"   Uptime % this month: {uptime_percent:.1f}%")
            print(f"   Version: {info.get('version', 'N/A')}")
    
    # Check for real-time throughput
    print("\n10. Real-time throughput:")
    health = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/health")
    if health.status_code == 200:
        data = health.json().get('data', [])
        for item in data:
            if item.get('subsystem') == 'wan':
                rx_rate = item.get('rx_bytes-r', 0)
                tx_rate = item.get('tx_bytes-r', 0)
                print(f"   Download: {(rx_rate * 8) / 1e6:.2f} Mbps")
                print(f"   Upload: {(tx_rate * 8) / 1e6:.2f} Mbps")
                
                # Check for uptime in health
                if 'uptime_stats' in item:
                    stats = item['uptime_stats']
                    if 'WAN' in stats:
                        wan_stats = stats['WAN']
                        print(f"   WAN availability: {wan_stats.get('availability', 0):.2f}%")
                        print(f"   WAN latency avg: {wan_stats.get('latency_average', 0):.1f} ms")