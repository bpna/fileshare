import subprocess
import sched, time
import threading
import os, shutil, glob



operator_port = '9054'


def clear_directory():
    
    folder = './'
    for the_file in os.listdir(folder):
        file_path = os.path.join(folder, the_file)
        try:
            if os.path.isfile(file_path) and the_file != 'test_script.py':
                os.unlink(file_path)
            elif os.path.isdir(file_path): shutil.rmtree(file_path)
        except Exception as e:
            print(e)

def pre_process():
    clear_directory()
    subprocess.run(['mkdir', 'serv_one'])

    subprocess.run(['mkdir', 'serv_two'])
    subprocess.run(['mkdir', 'cli_one'])
    subprocess.run(['mkdir', 'cli_two'])
    subprocess.run(['mkdir', 'output'])
    subprocess.run(['cp', '../server', './serv_one/'])
    subprocess.run(['cp', '../server', './serv_two/'])
    subprocess.run(['cp', '../client', './cli_one/'])
    subprocess.run(['cp', '../client', './cli_two/'])
    subprocess.run(['cp', '../operator', './'])
    subprocess.run(['cp', '../client.c', './file.file'])

    files = glob.glob('../*')
    for file in files:
        if os.path.isfile(file) and '.' in file:
            subprocess.run(['cp', file, './cli_one/'])
    # subprocess.run(['cp', '../file/file.file', './'])
    subprocess.run(['cp', '../client.o', './file.file2'])
    subprocess.run(['cp', './file.file', './cli_two/'])
    subprocess.run(['cp', './file.file2', './cli_two/'])
   

def run_operator():
    file = open("output/op_output.txt", "w")
    subprocess.run(['./operator', operator_port], stderr=file, stdout=file)
    file.close()

def run_server(server_name, portno):
    file = open("./output/{0}_output.txt".format(server_name), "w")
    subprocess.run([ 'valgrind','./server'.format(server_name), portno,server_name,'localhost', operator_port, "0"], stderr=file, stdout=file, cwd = '{0}/'.format(server_name))
    file.close()

def init_client(client_name):

    input_arr = ['valgrind', './client', 'init', 'localhost', operator_port]
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
    file.close()


    input_arr = ['valgrind','./client', 'new_client',  client_name, 'password' ]
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

#time.sleep(3)
#new_client commands
init_client('cli_one')
init_client('cli_two')

#cli one uploads
client_name = 'cli_one'

input_arr = ['./client', 'upload_file',  client_name, 'password',  'server.c']
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()


files = glob.glob('./cli_one/*')
for file in files:
    if os.path.isfile(file):
        local_file = file.split('cli_one/')[-1]
        input_arr = ['./client', 'upload_file',  client_name, 'password',  local_file]
        file = open("./output/{0}_output.txt".format(client_name), "a")
        subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
        file.close()

#cli_two checks out file
client_name = 'cli_two'
input_arr[2] = client_name
input_arr[1] = 'checkout_file'
input_arr[-1] = 'cli_one'
input_arr.append('server.c')
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()

# #cli_one checks out file (should fail)
# client_name = 'cli_one'
# input_arr[2] = client_name
# file = open("./output/{0}_output.txt".format(client_name), "a")
# subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
# file.close()


# #"modify" file
# subprocess.run(['cp', 'file.file', './cli_two/server.c'])

# #cli_two updates file
# client_name = 'cli_two'
# input_arr = ['./client', 'update_file', client_name, 'password', 'cli_one', 'server.c']
# file = open("./output/{0}_output.txt".format(client_name), "a")
# subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
# file.close()

#cli_two deletes file
client_name = 'cli_two'
input_arr = ['./client', 'delete_file', client_name, 'password', 'cli_one', 'server.c']
file = open("./output/{0}_output.txt".format(client_name), "a")
subprocess.run(input_arr, stderr=file, stdout=file, cwd = '{0}/'.format(client_name))
file.close()






# for thread in threads:
#    thread.join()

#psql commands:
#       sudo service postgresql start
#        psql fileshare
