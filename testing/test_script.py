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
    subprocess.run(['cp', '../file/file.file', './'])
    subprocess.run(['cp', '../file/file.file2', './'])
    subprocess.run(['cp', '../file/file.file', './cli_one'])
    subprocess.run(['cp', '../operator', './'])

def run_operator():
    file = open("output/op_output.txt", "w")
    subprocess.run(['./operator', operator_port], stderr=subprocess.STDOUT, stdout=file)
    file.close()

def run_server(server_name, portno):
    file = open("./output/{0}_output.txt".format(server_name), "w")
    subprocess.run(['./server'.format(server_name), portno,server_name,'localhost', operator_port], stderr=subprocess.STDOUT, stdout=file, cwd = '{0}/'.format(server_name))
    file.close()

def client_command(client_name):

    input_arr = ['./client', 'init', 'localhost', operator_port]
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=subprocess.STDOUT, stdout=file, cwd = '{0}/'.format(client_name))
    file.close()


    input_arr = ['./client', 'new_client',  client_name, 'password' ]
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=subprocess.STDOUT, stdout=file, cwd = '{0}/'.format(client_name))
    file.close()

    input_arr[1] = 'upload_file'
    input_arr.append('file.file')
    #cli one uploads
    if (client_name == 'cli_one'):
        file = open("./output/{0}_output.txt".format(client_name), "a")
        subprocess.run(input_arr, stderr=subprocess.STDOUT, stdout=file, cwd = '{0}/'.format(client_name))
        file.close()
        
        #cli one downloads, diffs
        #cli two downloads, diffs
        #cli one modifies
        #cli two downloads, diffs




pre_process()
op = threading.Thread(target=run_operator)
serv1 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_one', 'portno': '9060'})
serv2 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_two', 'portno': '9061'})

#new_client commands
cli1 = threading.Thread(target=client_command, kwargs = {'client_name': 'cli_one'})
cli2 = threading.Thread(target=client_command, kwargs = {'client_name': 'cli_two'})





threads = [op, serv1, serv2, cli1, cli2]

for thread in threads:
    thread.start()

for thread in threads:
    thread.join()

#psql commands:
#       sudo service postgresql start
#        psql fileshare