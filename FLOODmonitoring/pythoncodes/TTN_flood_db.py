import pymysql
import paho.mqtt.client as mqtt
import json
import time
import datetime
import pytz

# Get current time in UTC
utc_time = datetime.datetime.now(datetime.timezone.utc)  # Updated for timezone-aware UTC
# Convert UTC time to GMT+8
gmt8_timezone = pytz.timezone('Asia/Singapore')  # Change to the appropriate timezone

# Connect to the MySQL database
conn = pymysql.connect(
    host='localhost',
    user='root',
    password='ICPHpass!',
    database='flood_monitoring')

# Create a cursor object
cursor = conn.cursor()

# Define callback functions
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("v3/test-app-868-2@ttn/devices/test-id/up")

def on_message(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    
    if message.topic == "v3/test-app-868-2@ttn/devices/test-id/up":
        json_msg = json.loads(msg)
        
        try:
            # Check for 'uplink_message' and 'rx_metadata' keys first to avoid missing key errors
            uplink_message = json_msg.get('uplink_message', {})
            rx_metadata = uplink_message.get('rx_metadata', [{}])[0]  # Use [{}] as a fallback list with one empty dict
            decoded_payload = uplink_message.get('decoded_payload', {})
            
            # Extract each value with .get() and provide None as a fallback if the key is missing
            WaterLevel = decoded_payload.get('WaterLevel')
            BatteryLevel = decoded_payload.get('BatteryLevel')
            DeviceStatus = decoded_payload.get('DeviceStatus')
            RSSI = rx_metadata.get('rssi')
            SNR = rx_metadata.get('snr')
            packet_rec_ratio = "0"  # Default value or calculation
            
            # Check if all values are not None before proceeding
            if None not in (WaterLevel, BatteryLevel, DeviceStatus, RSSI, SNR):
                print(f"Battery Level: {BatteryLevel}")
                print(f"Water Level: {WaterLevel}")
                print(f"Device Status: {DeviceStatus}")
                print(f"RSSI: {RSSI}")
                print(f"SNR: {SNR}")
                
                # Call insert_data if all required fields are present
                insert_data(WaterLevel, DeviceStatus, RSSI, SNR, packet_rec_ratio, BatteryLevel)
            else:
                print("Incomplete data received, skipping database insert.")
                
        except Exception as e:
            print(f"An error occurred: {e}")

def insert_data(WaterLevel, DeviceStatus, RSSI, SNR, packet_rec_ratio, BatteryLevel):
    gmt8_time = datetime.datetime.now(pytz.timezone('Asia/Singapore'))
    TIMESTAMP = gmt8_time.strftime('%Y-%m-%d %H:%M:%S')
    # print("Current time in GMT+8:", TIMESTAMP)
    InsertDATA1 = "INSERT INTO brgy_nueva_era (timestamp, water_level, device_status, rssi_val, snr, packet_rec_ratio, battery_voltage) VALUES (%s, %s, %s, %s, %s, %s, %s)"
    try:
        cursor.execute(InsertDATA1, (TIMESTAMP, WaterLevel, DeviceStatus, RSSI, SNR, packet_rec_ratio, BatteryLevel))
        conn.commit()
        print('Insert into brgy_nueva_era Success')
    except Exception as e:
        print("An error occurred:", e)
        conn.rollback()

# Create a client instance
client = mqtt.Client()

# Set callback functions
client.on_connect = on_connect
client.on_message = on_message

# Set username and password
username = "test-app-868-2@ttn"
password = "NNSXS.HVL3QSKQKKC4XPFVIDQQDWYY6A2RVOMT4NOSI4A.RMBWGGVEATQLWZVO2IWODYTIV5M6QJ6USK2CJZIRM3AOBHFSRWVQ"
client.username_pw_set(username, password)

# Connect to the MQTT broker
client.connect("eu1.cloud.thethings.network", 1883, 60)

# Start the MQTT client loop
client.loop_forever()
