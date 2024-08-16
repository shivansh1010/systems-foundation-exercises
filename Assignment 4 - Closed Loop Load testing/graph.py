import pandas as pd
import matplotlib.pyplot as plt
import subprocess

port = 8080
init_users = 1
think_time = 0.005
testtime = 60
cores = [1,2,3]

throughput_arr = []
users_arr = []
rtt_list=[]

for users in range(init_users, 150, 10):
    try:
        args = "taskset -c " + ','.join(str(x) for x in cores) + " ./load_gen localhost "
        args = args + str(port) + " " + str(users) + " " + str(think_time) + " " + str(testtime)
        popen = subprocess.Popen(args.split(), stdout=subprocess.PIPE)
        popen.wait()
        output = popen.stdout.read().decode("utf-8").split()
    except :
        continue

    total_count = int(output[0])
    total_rtt = float(output[1])
    throughput = total_count/testtime
    avgrtt = total_rtt/total_count
    print (users, total_count, throughput,total_rtt, avgrtt)

    users_arr.append(users)
    throughput_arr.append(throughput)
    rtt_list.append(avgrtt)

    plt.subplot(2, 1, 1) 
    plt.plot(users_arr, throughput_arr)
    plt.title("Throughput plot")
    plt.xlabel('No of Concurrent users')
    plt.ylabel('Throughput (in reqs/s)')

    plt.subplot(2, 1, 2)
    plt.plot(users_arr, rtt_list)
    plt.title("Avg. RTT plot")
    plt.xlabel('No of Concurrent users')
    plt.ylabel('Average RTT (in secs)')

    plt.plot(users_arr, rtt_list)
    plt.tight_layout()
    plt.savefig("r7.jpg",bbox_inches="tight",pad_inches=0.4, transparent=True)
    
