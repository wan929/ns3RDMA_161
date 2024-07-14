from os import remove
import matplotlib.pyplot as plt
import numpy as np
def get_pctl(a, p):
	i = int(len(a) * p)
	return a[i]


font = {'family': 'Times New Roman',
        'weight': 'normal',
        'size': 18,
        }

font1 = {'family': 'Times New Roman',
        'weight': 'normal',
        'size': 18,
        }

file1 = open('/home/dell/wab/High-Precision-Congestion-Control-master/simulation/mix/fct_topology_fattree_k_8_flow_WebSearch_fattree_k_8_5ms_30load_dcqcn_1_1000_1_5000_400000_60_1.txt')  #打开文档
file2 = open('/home/dell/wab/High-Precision-Congestion-Control-master/simulation/mix/fct_topology_fattree_k_8_flow_WebSearch_fattree_k_8_5ms_30load_dcqcn_1_1000_1_5000_400000_60_0.txt')  #打开文档
# file3 = open('/home/wan/Desktop/High-Precision-Congestion-Control-master/simulation/mix/fct_topology_line_lg_flow_small_0.3load_0.2s_dcqcn_1_0_2_0_0_0_0.txt')  #打开文档
# file4 = open('/home/wan/Desktop/High-Precision-Congestion-Control-master/simulation/mix/fct_topology_line_lg_flow_small_0.9load_0.066s_dcqcn_1_0_2_0_0_0_1.txt')  #打开文档
data1 = file1.readlines()
data2 = file2.readlines()
# data3 = file3.readlines()
# data4 = file4.readlines()
tmp1 = []
tmp2 = []
# tmp3 = []
# tmp4 = []

for num1 in data1:
    tmp1.append([int(num1.split(' ')[4]),float(num1.split(' ')[6]),float(num1.split(' ')[7])])
for num2 in data2:
    tmp2.append([int(num2.split(' ')[4]),float(num2.split(' ')[6]),float(num2.split(' ')[7])])
# for num3 in data3:
#     tmp3.append([int(num3.split(' ')[4]),float(num3.split(' ')[6]),float(num3.split(' ')[7])])
# for num4 in data4:
#     tmp4.append([int(num4.split(' ')[4]),float(num4.split(' ')[6]),float(num4.split(' ')[7])])

r1 = []
r2 = []
r3 = []
r4 = []

for item in tmp1:
    r1.append(float(item[1] / 1000.0)) #fct
for item in tmp2:
    r2.append(float(item[1] / 1000.0)) #fct
# for item in tmp3:
#     r3.append(float(item[1] / 1000.0)) #fct
# for item in tmp4:
#     r4.append(float(item[1] / 1000.0)) #fct

fct1 = sorted(r1)
fct2 = sorted(r2)
# fct3 = sorted(r3)
# fct4 = sorted(r4)

cdf1 = np.arange(1, len(fct1) + 1) / len(fct1)
cdf2 = np.arange(1, len(fct2) + 1) / len(fct2)
# cdf3 = np.arange(1, len(fct3) + 1) / len(fct3)
# cdf4 = np.arange(1, len(fct4) + 1) / len(fct4)
plt.plot(fct1, cdf1, linestyle = '--', label = "origin")
plt.plot(fct2, cdf2, label = "INN")
# plt.plot(fct3, cdf3, label = "LinkGuardian")
# plt.plot(fct4, cdf4, label = "LinkGuardian_NB")
plt.legend()
ax = plt.gca()
ax.legend(prop = font1)
plt.tick_params(direction = 'in', top = False, bottom = True, left = True, right = False)
plt.xlabel('FCT(us)', fontproperties = font)
plt.ylabel('CDF', fontproperties = font)
plt.xlim(0, 1100)
plt.ylim(0, 1.00000001)
# plt.xticks(np.arange(0, 1100, 500), fontproperties = font1)
# plt.yticks(np.arange(0, 1.00000001, 0.2), fontproperties = font1)
plt.grid(True)
plt.gca().spines['right'].set_visible(False);
plt.gca().spines['top'].set_visible(False);

# plt.savefig('Plot/30load_small_0313.pdf', format = 'pdf' , dpi = 800 ,bbox_inches = 'tight')
plt.show()

# methods = ['origin', 'notification', 'LinkGuardian', 'LinkGuardian_NB']
# # 设置柱状图的宽度
# bar_width = 0.2
# index = np.arange(4)

# gap = 0.1

# # 画柱状图
# plt.bar(index, fct_avg, bar_width, label='average fct slowdown')
# plt.bar(index + bar_width, fct_50, bar_width, label='50% fct slowdown')
# plt.bar(index + 2 * bar_width, fct_95, bar_width, label='95% fct slowdown')
# plt.bar(index + 3 * bar_width, fct_99, bar_width, label='99% fct slowdown')
# plt.bar(index + 4 * bar_width, fct_max, bar_width, label='max fct slowdown')

# # 设置坐标轴标签和标题
# plt.xlabel('methods', fontproperties = font)
# plt.ylabel('fct slowdown', fontproperties = font)
# plt.title('90%load_small_packet_flow', fontproperties = font)
# plt.xticks(index + 2 * bar_width, methods, fontproperties = font1, rotation = 10)
# plt.yticks(fontproperties = font1)
# plt.legend()

# # 显示图表
# #plt.savefig('Plot/90load_small_0204.pdf', format = 'pdf' , dpi = 800 ,bbox_inches = 'tight')

# plt.show()