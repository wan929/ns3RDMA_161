from os import remove

def get_pctl(a, p):
	i = int(len(a) * p)
	return a[i]

file = open('/home/dell/wab/High-Precision-Congestion-Control-master/simulation/mix/fct_topology_fattree_k_8_flow_test_fh_3m_dcqcn_1_1000_1_5000_400000_60_0.txt')  #打开文档
data = file.readlines()
tmp = []
a = []
for num in data:
    tmp.append([str(str(num.split(' ')[0])+str(num.split(' ')[1])+str(num.split(' ')[2])+str(num.split(' ')[3])),int(num.split(' ')[4]),float(num.split(' ')[6]),float(num.split(' ')[7])])

r = []
r_1 = []
r_7 = []
r_46 = []
r_120 = []
r_300 = []
r_1000 = []
r_10000 = []

for item in tmp:
    r.append([int(-(-item[1]//1000)), float(item[2]/item[3])]) #pn,fct_slowdown

for it in r:
    if it[1] < 1:
        it[1] = 1

for i in r:
    if i[0] <= 1:
        r_1.append(i)
    elif i[0] <= 7:
        r_7.append(i)
    elif i[0] <= 46:
        r_46.append(i)
    elif i[0] <= 120:
        r_120.append(i)
    elif i[0] <= 300:
        r_300.append(i)
    elif i[0] <= 1000:
        r_1000.append(i)
    elif i[0] <= 10000:
        r_10000.append(i)

r_1.sort(key=lambda x:x[1])
r_7.sort(key=lambda x:x[1])
r_46.sort(key=lambda x:x[1])
r_120.sort(key=lambda x:x[1])
r_300.sort(key=lambda x:x[1])
r_1000.sort(key=lambda x:x[1])
r_10000.sort(key=lambda x:x[1])

fct_avg = []
fct_50 = []
fct_95 = []
fct_99 = []
fct_max = []

r_lists = [r_1, r_7, r_46, r_120, r_300, r_1000, r_10000]
for r_list in r_lists:
    fct = sorted(map(lambda x: x[1], r_list))
    fct_avg.append(sum(fct) / len(fct))
    fct_50.append(get_pctl(fct, 0.5))
    fct_95.append(get_pctl(fct, 0.95))
    fct_99.append(get_pctl(fct, 0.99))
    fct_max.append(fct[-1])

fct_lists = [fct_avg, fct_50, fct_95, fct_99, fct_max]
for fct_list in fct_lists:
    line = "%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f"%(fct_list[0], fct_list[1], fct_list[2], fct_list[3], fct_list[4], fct_list[5], fct_list[6])
    print(line)