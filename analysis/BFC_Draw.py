import csv
import numpy as np
from matplotlib import pyplot as plt
import matplotlib
import argparse

font = {'family': 'Times New Roman',
        'weight': 'normal',
        'size': 20,
        }
font1 = {'family': 'Times New Roman',
        'weight': 'normal',
        'size': 20,
        }


with open('BFC_compare.csv','r') as f:
    reader = csv.reader(f)
    result = list(reader)
    DL_ave = result[1]
    DL_medi = result[2]
    DL_95 = result[3]
    DL_99 = result[4]
    DL_max = result[5]
    BFC_ave = result[6]
    BFC_medi = result[7]
    BFC_95 = result[8]
    BFC_99 = result[9]
    BFC_max = result[10]
    # DL_NUTS_ave = result[16]
    # DL_NUTS_50 = result[17]
    # DL_NUTS_95 = result[18]
    # DL_NUTS_99 = result[19]
    # DL_NUTS_max = result[20]
    # BFC_onequeue_ave = result[21]
    # BFC_onequeue_medi = result[22]
    # BFC_onequeue_95 = result[23]
    # BFC_onequeue_99 = result[24]
    # BFC_onequeue_max = result[25]

DL1= [float(x) for x in DL_ave]
DL2= [float(x) for x in DL_medi]
DL3= [float(x) for x in DL_95]
DL4= [float(x) for x in DL_99]
DL5= [float(x) for x in DL_max]
BFC1= [float(x) for x in BFC_ave]
BFC2= [float(x) for x in BFC_medi]
BFC3= [float(x) for x in BFC_95]
BFC4= [float(x) for x in BFC_99]
BFC5= [float(x) for x in BFC_max]
# DL_NUTS_1= [float(x) for x in DL_NUTS_ave]
# DL_NUTS_2= [float(x) for x in DL_NUTS_50]
# DL_NUTS_3= [float(x) for x in DL_NUTS_95]
# DL_NUTS_4= [float(x) for x in DL_NUTS_99]
# DL_NUTS_5= [float(x) for x in DL_NUTS_max]
# BFC_onequeue_1= [float(x) for x in BFC_onequeue_ave]
# BFC_onequeue_2= [float(x) for x in BFC_onequeue_medi]
# BFC_onequeue_3= [float(x) for x in BFC_onequeue_95]
# BFC_onequeue_4= [float(x) for x in BFC_onequeue_99]
# BFC_onequeue_5= [float(x) for x in BFC_onequeue_max]


x = [0,1,2,3,4,5,6,7,8]

# plt.plot(x,DL1, 'b-',alpha=0.5, linewidth=3, label='Mine_avg')
# plt.plot(x,DL2, 'b--',alpha=0.5, linewidth=3, label='Mine_50')
plt.plot(x,DL3, 'b-',alpha=0.5, linewidth=3, label='Mine_95')
plt.plot(x,DL4, 'b--',alpha=0.5, linewidth=3, label='Mine_99')
# plt.plot(x,DL5, 'b-',alpha=0.5, linewidth=3, label='Mine_max')
# plt.plot(x,BFC1,'r-',alpha=0.5, linewidth=3, label='BFC_avg')
# plt.plot(x,BFC2,'r--',alpha=0.5, linewidth=3, label='BFC_50')
plt.plot(x,BFC3,'r-',alpha=0.5, linewidth=3, label='BFC_95')
plt.plot(x,BFC4,'r--',alpha=0.5, linewidth=3, label='BFC_99')
# plt.plot(x,BFC5, 'r-',alpha=0.5, linewidth=3, label='BFC_max')
# plt.plot(x,DL_NUTS_1, 'k-',alpha=0.5, linewidth=3, label='Mine_NUTS_avg')
# plt.plot(x,DL_NUTS_2, 'k--',alpha=0.5, linewidth=3, label='Mine_NUTS_50')
# plt.plot(x,DL_NUTS_3, 'k-',alpha=0.5, linewidth=3, label='Mine_NUTS_95')
# plt.plot(x,DL_NUTS_4, 'k--',alpha=0.5, linewidth=3, label='Mine_NUTS_99')
# plt.plot(x,DL_NUTS_5, 'k-',alpha=0.5, linewidth=3, label='Mine_NUTS_max')
# plt.plot(x,BFC_onequeue_1,'r-',alpha=0.5, linewidth=3, label='BFC_onequeue_avg')
# plt.plot(x,BFC_onequeue_2,'r--',alpha=0.5, linewidth=3, label='BFC_onequeue_50')
# plt.plot(x,BFC_onequeue_3,'r-',alpha=0.5, linewidth=3, label='BFC_onequeue_95')
# plt.plot(x,BFC_onequeue_4,'r--',alpha=0.5, linewidth=3, label='BFC_onequeue_99')
# plt.plot(x,BFC_onequeue_5, 'r-',alpha=0.5, linewidth=3, label='BFC_onequeue_max')





plt.legend()
ax = plt.gca()

ax.legend(loc = 'best' , ncol = 1 , prop = font1)

plt.xlabel('Flow size(packets number)', fontproperties=font)
plt.ylabel('FCT', fontproperties=font)

plt.xlim(1, 9)
plt.xticks(np.arange(9),(2**0,2**1,2**2,2**3,2**4,2**6,2**8,2**10,1500),fontproperties=font1)
# plt.ylim(0, 1)
plt.title('Google_60%_5ms',font)
plt.savefig('Picture/FCT.pdf' , format = 'pdf' , dpi = 800 ,bbox_inches = 'tight')
plt.show()
