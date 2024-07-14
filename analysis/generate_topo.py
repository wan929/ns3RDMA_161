import os
import numpy as np
with open("topologyfattree4.txt" , 'a') as f:
    for i in range(128,143):
         f.write(str(i)+" ")
    for i in range(136,144):
         for j in range(128,136):
             f.writelines(str(i)+" "+str(j)+" "+"100Gbps 0.001ms 0\n")
    for i in range(128,136):
        for j in range((i-128)*16,(i-127)*16):
            f.writelines(str(i)+" "+str(j)+" "+"100Gbps 0.001ms 0\n")
