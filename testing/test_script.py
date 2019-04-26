import subprocess
import sched, time
import threading



operator_port = '9054'

def pre_process():
    subprocess.run(['rm', '-vrf', '!("test_script.py"|"test_init.py")'])
    subprocess.run(['mkdir', 'serv_one'])

    subprocess.run(['mkdir', 'serv_two'])
    subprocess.run(['mkdir', 'cli_one'])
    subprocess.run(['mkdir', 'cli_two'])
    subprocess.run(['mkdir', 'output'])
    subprocess.run(['cp', '../server', './serv_one/'])
    subprocess.run(['cp', '../server', './serv_two/'])
    subprocess.run(['cp', '../client', './cli_one/'])
    subprocess.run(['cp', '../client', './cli_two/'])
    subprocess.run(['cp', '../client.c', './file.file'])
    # subprocess.run(['cp', '../file/file.file', './'])
    subprocess.run(['cp', '../client.o', './file.file2'])
    subprocess.run(['cp', './file.file', './cli_one/'])
    subprocess.run(['cp', '../operator', './'])

def run_operator():
    file = open("output/op_output.txt", "w")
    subprocess.run(['./operator', operator_port], stderr=file, stdout=file)
    file.close()

def run_server(server_name, portno):
    file = open("./output/{0}_output.txt".format(server_name), "w")
    subprocess.run([ 'valgrind','./server'.format(server_name), portno,server_name,'localhost', operator_port, "0"], stderr=file, stdout=file, cwd = '{0}/'.format(server_name))
    file.close()

def init_client(client_name):

    input_arr = ['./client', 'init', 'localhost', operator_port]
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
    file.close()


    input_arr = ['./client', 'new_client',  client_name, 'password' ]
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
    file.close()





pre_process()
op = threading.Thread(target=run_operator)
serv1 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_one', 'portno': '9060'})
serv2 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_two', 'portno': '9061'})







threads = [op, serv1, serv2]

for thread in threads:
    thread.start()

time.sleep(5)
#new_client commands
init_client('cli_one')
init_client('cli_two')

#cli one uploads
client_name = 'cli_one'
input_arr = ['./client', 'upload_file',  client_name, 'password', 'file.file' ]

file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()

#cli_two checks out file
client_name = 'cli_two'
input_arr[2] = client_name
input_arr[1] = 'checkout_file'
input_arr[-1] = 'cli_one'
input_arr.append('file.file')
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()

#cli_one checks out file (should fail)
client_name = 'cli_one'
input_arr[2] = client_name
input_arr.insert(0, '--leak-check=full')

input_arr.insert(0, 'valgrind')
print (str(input_arr))
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()


# #"modify" file
# subprocess.run(['cp', 'file.file2', './cli_two/file.file'])

# #cli_two updates file
# client_name = 'cli_two'
# input_arr = ['./client', 'update_file', client_name, 'password', 'cli_one', 'file.file']
# file = open("./output/{0}_output.txt".format(client_name), "a")
# subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
# file.close()

#cli_two deletes file
client_name = 'cli_two'
input_arr = ['./client', 'delete_file', client_name, 'password', 'cli_one', 'file.file']
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()






# for thread in threads:
#    thread.join()

#psql commands:
#       sudo service postgresql start
#        psql fileshare
