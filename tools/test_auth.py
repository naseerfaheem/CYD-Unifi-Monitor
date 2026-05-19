#!/usr/bin/env python3
"""Test UniFi authentication flow"""

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

# Create a session to persist cookies
session = requests.Session()
session.verify = False

print("=== Testing UniFi Authentication ===")
print(f"URL: {BASE_URL}/api/auth/login")

# Login request
login_data = {"username": UNIFI_USERNAME, "password": UNIFI_PASSWORD, "remember": False}
print(f"Request body: {json.dumps(login_data)}")

response = session.post(f"{BASE_URL}/api/auth/login", json=login_data)
print(f"\nResponse status: {response.status_code}")
print(f"Response headers:")
for key, value in response.headers.items():
    if key.lower() in ['set-cookie', 'x-csrf-token', 'content-type']:
        print(f"  {key}: {value[:100]}...")

print(f"\nCookies in session:")
for cookie in session.cookies:
    print(f"  {cookie.name} = {cookie.value[:50]}...")
    
print(f"\nResponse body (first 500 chars):")
print(response.text[:500])

# Test an API call
print("\n=== Testing API call with session ===")
test_response = session.get(f"{BASE_URL}/proxy/network/api/s/default/stat/health")
print(f"Health endpoint status: {test_response.status_code}")
if test_response.status_code == 200:
    data = test_response.json()
    print(f"Success! Got {len(data.get('data', []))} health items")
else:
    print(f"Failed: {test_response.text[:200]}")