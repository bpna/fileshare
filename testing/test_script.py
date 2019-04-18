import subprocess
import sched, time
import threading



operator_port = '9054'

def pre_process():
    subprocess.run(['rm', '-vrf', '!("test_script.py")'])
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

def run_server(server_name):
    file = open("./output/{0}_output.txt".format(server_name), "w")
    subprocess.run(['{0}/server'.format(server_name), '9060',server_name,'localhost', operator_port], stderr=subprocess.STDOUT, stdout=file)
    file.close()

def client_command(client_name, input_arr):
    file = open("./output/{0}_output.txt".format(client_name), "a")
    subprocess.run(input_arr, stderr=subprocess.STDOUT, stdout=file)
    file.close()



pre_process()
op = threading.Thread(target=run_operator)
serv1 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_one'})
serv2 = threading.Thread(target=run_server, kwargs = {'server_name': 'serv_two'})
cli_name = 'cli_one'
cli_arr = ['{0}/client'.format(cli_name), 'new_client', 'localhost', operator_port, cli_name, 'password' ]
cli1_start = threading.Thread(target=client_command, kwargs = {'client_name': 'cli_one', 'input_arr': cli_arr})
cli_name = 'cli_two'
cli_arr[0] = '{0}/client'.format(cli_name)
cli2_start = threading.Thread(target=client_command, kwargs = {'client_name': 'cli_two', 'input_arr': cli_arr})


#cli one uploads
cli_name = 'cli_one'
cli_arr[0] = '{0}/client'.format(cli_name)
cli_arr[1] = 'upload_file'
cli_arr.append('file.file')
cli_one_upload = threading.Thread(target=client_command, kwargs = {'client_name': 'cli_one', 'input_arr': cli_arr})

#cli one downloads, diffs
#cli two downloads, diffs
#cli one modifies
#cli two downloads, diffs



threads = [op, serv1, cli1_start, cli2_start, cli_one_upload]

for thread in threads:
    thread.start()

for thread in threads:
    thread.join()

#psql commands:
#       sudo service postgresql start
#        psql fileshare