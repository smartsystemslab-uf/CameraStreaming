import paramiko
import time
import datetime
import json


# -------------------- Configuration --------------------
#                  Cam-1            Cam-2           Cam-3         Cam-4           Cam-5           Cam-6         Cam-7           Cam-8           Cam-9
ip_addresses = [" "ip1", "ip2", "ip3", "ip4", "ip5", "ip6", "ip7", "ip8", "ip9" "] #ip address
username = 'username'    # SSH username
password = 'password'    # SSH password
command = 'command'    # Command to execute
c_file = "config_file"    #path to JSON config_file
server_ip = 'serveur_ip'  #serveur ip addresse
loop_time = 1800      #time between each update in seconde
# -------------------- Configuration --------------------


def ssh_command(ip, username, password, command):
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(ip, username=username, password=password)
        stdin, stdout, stderr = client.exec_command(command)
        output = stdout.read().decode()
        error = stderr.read().decode()
        client.close()
        return output, error
    except paramiko.AuthenticationException:
        return None, f"Authentication failed for {ip}"
    except paramiko.SSHException as e:
        return None, f"SSH connection failed for {ip}: {str(e)}"
    except paramiko.Exception as e:
        return None, f"An error occurred while connecting to {ip}: {str(e)}"

def execute_command_on_multiple_ips(ips, username, password, command, config_file):
    formatted_data = []

    for ip in ips:
        output, error = ssh_command(ip, username, password, command)
        if output:
            formatted_item = {
                "deviceInfo": {
                    "con_mac_addr": output.strip(),
                    "last_update": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    "local_ip_addr": ip,
                    "mac_addr": "B8:27:EB:3C:6B:0A"
                },
                "last_update": datetime.datetime(2021, 10, 17, 9, 37, 54).strftime("%Y-%m-%d %H:%M:%S"),
                "serverip": server_ip,
                "serverport": 9434
            }
            formatted_data.append(formatted_item)

        if error:
            print(f"Error for {ip}:")
            print(error)

    json_data = {
        "clientdata": formatted_data,
        "serverport": 9434
    }

    with open(config_file, "w") as file:
        json.dump(json_data, file, indent=4)


#while True:
execute_command_on_multiple_ips(ip_addresses, username, password, command, c_file)
    #time.sleep(loop_time)
