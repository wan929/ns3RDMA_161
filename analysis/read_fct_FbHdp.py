from os import remove


file1 = open('/home/wan/Desktop/in-network_notification/2500_1/timely/init/fct_topology_fattree_k_8_flow_WebSearch_fattree_k_8_0.005s_0.3load_timely.txt')  #打开文档
file2 = open('/home/wan/Desktop/in-network_notification/2500_1/timely/mine/fct_topology_fattree_k_8_flow_WebSearch_fattree_k_8_0.005s_0.3load_timely.txt')  #打开文档
#file3 = open('/home/wan/Desktop/fct_topology_fattree_k_8_flow_WebSearch_fattree_k_8_0.0005s_0.3load_1000_1_timely(3).txt')  #打开文档
data1 = file1.readlines() #读取文档数据
data2 = file2.readlines() #读取文档数据
#data3 = file3.readlines() #读取文档数据
tmp1 = []
tmp2 = []

a = []
b = []
for num1 in data1:
    tmp1.append([str(str(num1.split(' ')[0])+str(num1.split(' ')[1])+str(num1.split(' ')[2])+str(num1.split(' ')[3])),int(num1.split(' ')[4]),int(num1.split(' ')[6]),int(num1.split(' ')[7])])
    a.append(str(str(num1.split(' ')[0])+str(num1.split(' ')[1])+str(num1.split(' ')[2])+str(num1.split(' ')[3])))
for num2 in data2:
    tmp2.append([str(str(num2.split(' ')[0])+str(num2.split(' ')[1])+str(num2.split(' ')[2])+str(num2.split(' ')[3])),int(num2.split(' ')[4]),int(num2.split(' ')[6]),int(num2.split(' ')[7])])
    b.append(str(str(num2.split(' ')[0])+str(num2.split(' ')[1])+str(num2.split(' ')[2])+str(num2.split(' ')[3])))
#for num3 in data3:
 #   tmp3.append([int(num3.split(' ')[4]),int(num3.split(' ')[6]),int(num3.split(' ')[7])])
  #  b.append(int(num3.split(' ')[4]))

c = [v for v in a if v in b] # &
d = [v for v in b if v in a] # &

#for d in c:
   # print (d)
#a_set = list(set(a))
#b_set = list(set(b))
#c_set = list(set(c))
#d_set = list(set(d))
#print (len(tmp1))
#print (len(tmp2))
#print (len(a))
#print (len(b))
#print (len(a_set))
#print (len(b_set))
#print (len(c))
#print (len(d))
#print (len(c_set))
#print (len(d_set))

tmp3 = []
tmp4 = []
'''
for it1 in c:
    for it2 in tmp1:
        if it2[0] == it1:
            tmp3.append(it2)

for it1 in c:
    for it2 in tmp2:
        if it2[0] == it1:
            tmp4.append(it2)
'''
'''
for it1 in tmp1:
    flag = 0
    for it2 in c:
        if it1[0] == it2:
            flag = 1
    if flag == 0:
        tmp1.remove(it1)

for it1 in tmp2:
    flag = 0
    for it2 in c:
        if it1[0] == it2:
            flag = 1
    if flag == 0:
        tmp2.remove(it1)
'''
#print (len(tmp3))
#print (len(tmp4))
tmp3.sort(key=lambda x:(x[1],x[3]))
tmp4.sort(key=lambda x:(x[1],x[3]))
#tmp3.sort(key=lambda x:x[0])

#print (len(tmp2))
#print (len(tmp3))


#normalist1 = []
#normalist2 = []
#normalist3 = []
#for normalnum1, normalnum2, normalnum3 in zip(tmp1,tmp2,tmp3):
   # normalist1.append([int(normalnum1[0]), float(normalnum1[1]/normalnum1[2])])
   # normalist2.append([int(normalnum2[0]), float(normalnum2[1]/normalnum2[2])])
   # normalist3.append([int(normalnum3[0]), float(normalnum3[1]/normalnum3[2])])
#for item1, item2, item3 in zip(normalist1,normalist2,normalist3):
    #line = "%d,%f,%f,%f"%(item1[0], item1[1], item2[1],item3[1])
    #print (line)

#for item1, item2, item3 in zip(tmp1,tmp2,tmp3):
#    line = "%d,%.3f,%.3f,%.3f"%(item1[0], item1[1]/1000, item2[1]/1000, item3[1]/1000)
#    print (line)
r_init = []
r_mine = []
r_1_init = []
r_2_init = []
r_7_init = []
r_30_init = []
r_50_init = []
r_80_init = []
r_120_init = []
r_300_init = []
r_1000_init = []
r_2000_init = []
r_10000_init = []
r_1_mine = []
r_2_mine = []
r_7_mine = []
r_30_mine = []
r_50_mine = []
r_80_mine = []
r_120_mine = []
r_300_mine = []
r_1000_mine = []
r_2000_mine = []
r_10000_mine = []
'''
for item1, item2 in zip(tmp3,tmp4):
    r.append([-(-item1[1]//1000),-(-item2[1]//1000),item1[2]/1000,item2[2]/1000])
    #line = "%d,%d,%.3f,%.3f"%(-(-item1[1]//1000),-(-item2[1]//1000),item1[2]/1000,item2[2]/1000)
    #print (line)
'''
for item1 in tmp1:
    r_init.append([int(-(-item1[1]//1000)), float(item1[2]/1000), float(item1[3]/1000)]) #pn,fct(us),s_fct(us)
for item2 in tmp2:
    r_mine.append([int(-(-item2[1]//1000)), float(item2[2]/1000), float(item2[3]/1000)]) #pn,fct(us),s_fct(us)

for i in r_init:
    if i[0] <= 1:
        r_1_init.append(i)
    elif i[0] <= 2:
        r_2_init.append(i)
    elif i[0] <= 7:
        r_7_init.append(i)
    elif i[0] <= 30:
        r_30_init.append(i)
    elif i[0] <= 50:
        r_50_init.append(i)
    elif i[0] <= 80:
        r_80_init.append(i)
    elif i[0] <= 120:
        r_120_init.append(i)
    elif i[0] <= 300:
        r_300_init.append(i)
    elif i[0] <= 1000:
        r_1000_init.append(i)
    elif i[0] <= 2000:
        r_2000_init.append(i)
    elif i[0] <= 10000:
        r_10000_init.append(i)

for i in r_mine:
    if i[0] <= 1:
        r_1_mine.append(i)
    elif i[0] <= 2:
        r_2_mine.append(i)
    elif i[0] <= 7:
        r_7_mine.append(i)
    elif i[0] <= 30:
        r_30_mine.append(i)
    elif i[0] <= 50:
        r_50_mine.append(i)
    elif i[0] <= 80:
        r_80_mine.append(i)
    elif i[0] <= 120:
        r_120_mine.append(i)
    elif i[0] <= 300:
        r_300_mine.append(i)
    elif i[0] <= 1000:
        r_1000_mine.append(i)
    elif i[0] <= 2000:
        r_2000_mine.append(i)
    elif i[0] <= 10000:
        r_10000_mine.append(i)

# 1
len_1_init = len(r_1_init)
p_50_1_init = int(len_1_init*0.5)
p_95_1_init = int(len_1_init*0.95)
p_99_1_init = int(len_1_init*0.99)
sum_init_1 = 0
for x in r_1_init:
    sum_init_1 += x[1]
#print(sum_init/len(r_1))
#print(sum_mine/len(r_1))
r_1_init.sort(key=lambda x:x[1])
init_50_1 = r_1_init[p_50_1_init][1]
init_95_1 = r_1_init[p_95_1_init][1]
init_99_1 = r_1_init[p_99_1_init][1]
#print(r_1[p_95][2])
#print(r_1[p_99][2])
len_1_mine = len(r_1_mine)
p_50_1_mine = int(len_1_mine*0.5)
p_95_1_mine = int(len_1_mine*0.95)
p_99_1_mine = int(len_1_mine*0.99)
sum_mine_1 = 0
for x in r_1_mine:
    sum_mine_1 += x[1]
r_1_mine.sort(key=lambda x:x[1])
mine_50_1 = r_1_mine[p_50_1_mine][1]
mine_95_1 = r_1_mine[p_95_1_mine][1]
mine_99_1 = r_1_mine[p_99_1_mine][1]
#print(r_1[p_95][3])
#print(r_1[p_99][3])
line_1 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(1,sum_init_1/len_1_init,sum_mine_1/len_1_mine,init_50_1,mine_50_1,init_95_1,mine_95_1,init_99_1,mine_99_1)
print (line_1)

# 2
len_2_init = len(r_2_init)
p_50_2_init = int(len_2_init*0.5)
p_95_2_init = int(len_2_init*0.95)
p_99_2_init = int(len_2_init*0.99)
sum_init_2 = 0
for x in r_2_init:
    sum_init_2 += x[1]
#print(sum_init/len(r_2))
#print(sum_mine/len(r_2))
r_2_init.sort(key=lambda x:x[1])
init_50_2 = r_2_init[p_50_2_init][1]
init_95_2 = r_2_init[p_95_2_init][1]
init_99_2 = r_2_init[p_99_2_init][1]
#print(r_2[p_95][2])
#print(r_2[p_99][2])
len_2_mine = len(r_2_mine)
p_50_2_mine = int(len_2_mine*0.5)
p_95_2_mine = int(len_2_mine*0.95)
p_99_2_mine = int(len_2_mine*0.99)
sum_mine_2 = 0
for x in r_2_mine:
    sum_mine_2 += x[1]
r_2_mine.sort(key=lambda x:x[1])
mine_50_2 = r_2_mine[p_50_2_mine][1]
mine_95_2 = r_2_mine[p_95_2_mine][1]
mine_99_2 = r_2_mine[p_99_2_mine][1]
#print(r_2[p_95][3])
#print(r_2[p_99][3])
line_2 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(2,sum_init_2/len_2_init,sum_mine_2/len_2_mine,init_50_2,mine_50_2,init_95_2,mine_95_2,init_99_2,mine_99_2)
print (line_2)

# 7
len_7_init = len(r_7_init)
p_50_7_init = int(len_7_init*0.5)
p_95_7_init = int(len_7_init*0.95)
p_99_7_init = int(len_7_init*0.99)
sum_init_7 = 0
for x in r_7_init:
    sum_init_7 += x[1]
#print(sum_init/len(r_7))
#print(sum_mine/len(r_7))
r_7_init.sort(key=lambda x:x[1])
init_50_7 = r_7_init[p_50_7_init][1]
init_95_7 = r_7_init[p_95_7_init][1]
init_99_7 = r_7_init[p_99_7_init][1]
#print(r_7[p_95][2])
#print(r_7[p_99][2])
len_7_mine = len(r_7_mine)
p_50_7_mine = int(len_7_mine*0.5)
p_95_7_mine = int(len_7_mine*0.95)
p_99_7_mine = int(len_7_mine*0.99)
sum_mine_7 = 0
for x in r_7_mine:
    sum_mine_7 += x[1]
r_7_mine.sort(key=lambda x:x[1])
mine_50_7 = r_7_mine[p_50_7_mine][1]
mine_95_7 = r_7_mine[p_95_7_mine][1]
mine_99_7 = r_7_mine[p_99_7_mine][1]
#print(r_7[p_95][3])
#print(r_7[p_99][3])
line_7 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(7,sum_init_7/len_7_init,sum_mine_7/len_7_mine,init_50_7,mine_50_7,init_95_7,mine_95_7,init_99_7,mine_99_7)
print (line_7)

# 30
len_30_init = len(r_30_init)
p_50_30_init = int(len_30_init*0.5)
p_95_30_init = int(len_30_init*0.95)
p_99_30_init = int(len_30_init*0.99)
sum_init_30 = 0
for x in r_30_init:
    sum_init_30 += x[1]
#print(sum_init/len(r_30))
#print(sum_mine/len(r_30))
r_30_init.sort(key=lambda x:x[1])
init_50_30 = r_30_init[p_50_30_init][1]
init_95_30 = r_30_init[p_95_30_init][1]
init_99_30 = r_30_init[p_99_30_init][1]
#print(r_30[p_95][2])
#print(r_30[p_99][2])
len_30_mine = len(r_30_mine)
p_50_30_mine = int(len_30_mine*0.5)
p_95_30_mine = int(len_30_mine*0.95)
p_99_30_mine = int(len_30_mine*0.99)
sum_mine_30 = 0
for x in r_30_mine:
    sum_mine_30 += x[1]
r_30_mine.sort(key=lambda x:x[1])
mine_50_30 = r_30_mine[p_50_30_mine][1]
mine_95_30 = r_30_mine[p_95_30_mine][1]
mine_99_30 = r_30_mine[p_99_30_mine][1]
#print(r_30[p_95][3])
#print(r_30[p_99][3])
line_30 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(30,sum_init_30/len_30_init,sum_mine_30/len_30_mine,init_50_30,mine_50_30,init_95_30,mine_95_30,init_99_30,mine_99_30)
print (line_30)

# 50
len_50_init = len(r_50_init)
p_50_50_init = int(len_50_init*0.5)
p_95_50_init = int(len_50_init*0.95)
p_99_50_init = int(len_50_init*0.99)
sum_init_50 = 0
for x in r_50_init:
    sum_init_50 += x[1]
#print(sum_init/len(r_50))
#print(sum_mine/len(r_50))
r_50_init.sort(key=lambda x:x[1])
init_50_50 = r_50_init[p_50_50_init][1]
init_95_50 = r_50_init[p_95_50_init][1]
init_99_50 = r_50_init[p_99_50_init][1]
#print(r_50[p_95][2])
#print(r_50[p_99][2])
len_50_mine = len(r_50_mine)
p_50_50_mine = int(len_50_mine*0.5)
p_95_50_mine = int(len_50_mine*0.95)
p_99_50_mine = int(len_50_mine*0.99)
sum_mine_50 = 0
for x in r_50_mine:
    sum_mine_50 += x[1]
r_50_mine.sort(key=lambda x:x[1])
mine_50_50 = r_50_mine[p_50_50_mine][1]
mine_95_50 = r_50_mine[p_95_50_mine][1]
mine_99_50 = r_50_mine[p_99_50_mine][1]
#print(r_50[p_95][3])
#print(r_50[p_99][3])
line_50 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(50,sum_init_50/len_50_init,sum_mine_50/len_50_mine,init_50_50,mine_50_50,init_95_50,mine_95_50,init_99_50,mine_99_50)
print (line_50)

# 80
len_80_init = len(r_80_init)
p_50_80_init = int(len_80_init*0.5)
p_95_80_init = int(len_80_init*0.95)
p_99_80_init = int(len_80_init*0.99)
sum_init_80 = 0
for x in r_80_init:
    sum_init_80 += x[1]
#print(sum_init/len(r_80))
#print(sum_mine/len(r_80))
r_80_init.sort(key=lambda x:x[1])
init_50_80 = r_80_init[p_50_80_init][1]
init_95_80 = r_80_init[p_95_80_init][1]
init_99_80 = r_80_init[p_99_80_init][1]
#print(r_80[p_95][2])
#print(r_80[p_99][2])
len_80_mine = len(r_80_mine)
p_50_80_mine = int(len_80_mine*0.5)
p_95_80_mine = int(len_80_mine*0.95)
p_99_80_mine = int(len_80_mine*0.99)
sum_mine_80 = 0
for x in r_80_mine:
    sum_mine_80 += x[1]
r_80_mine.sort(key=lambda x:x[1])
mine_50_80 = r_80_mine[p_50_80_mine][1]
mine_95_80 = r_80_mine[p_95_80_mine][1]
mine_99_80 = r_80_mine[p_99_80_mine][1]
#print(r_80[p_95][3])
#print(r_80[p_99][3])
line_80 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(80,sum_init_80/len_80_init,sum_mine_80/len_80_mine,init_50_80,mine_50_80,init_95_80,mine_95_80,init_99_80,mine_99_80)
print (line_80)

# 120
len_120_init = len(r_120_init)
p_50_120_init = int(len_120_init*0.5)
p_95_120_init = int(len_120_init*0.95)
p_99_120_init = int(len_120_init*0.99)
sum_init_120 = 0
for x in r_120_init:
    sum_init_120 += x[1]
#print(sum_init/len(r_120))
#print(sum_mine/len(r_120))
r_120_init.sort(key=lambda x:x[1])
init_50_120 = r_120_init[p_50_120_init][1]
init_95_120 = r_120_init[p_95_120_init][1]
init_99_120 = r_120_init[p_99_120_init][1]
#print(r_120[p_95][2])
#print(r_120[p_99][2])
len_120_mine = len(r_120_mine)
p_50_120_mine = int(len_120_mine*0.5)
p_95_120_mine = int(len_120_mine*0.95)
p_99_120_mine = int(len_120_mine*0.99)
sum_mine_120 = 0
for x in r_120_mine:
    sum_mine_120 += x[1]
r_120_mine.sort(key=lambda x:x[1])
mine_50_120 = r_120_mine[p_50_120_mine][1]
mine_95_120 = r_120_mine[p_95_120_mine][1]
mine_99_120 = r_120_mine[p_99_120_mine][1]
#print(r_120[p_95][3])
#print(r_120[p_99][3])
line_120 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(120,sum_init_120/len_120_init,sum_mine_120/len_120_mine,init_50_120,mine_50_120,init_95_120,mine_95_120,init_99_120,mine_99_120)
print (line_120)

# 300
len_300_init = len(r_300_init)
p_50_300_init = int(len_300_init*0.5)
p_95_300_init = int(len_300_init*0.95)
p_99_300_init = int(len_300_init*0.99)
sum_init_300 = 0
for x in r_300_init:
    sum_init_300 += x[1]
#print(sum_init/len(r_300))
#print(sum_mine/len(r_300))
r_300_init.sort(key=lambda x:x[1])
init_50_300 = r_300_init[p_50_300_init][1]
init_95_300 = r_300_init[p_95_300_init][1]
init_99_300 = r_300_init[p_99_300_init][1]
#print(r_300[p_95][2])
#print(r_300[p_99][2])
len_300_mine = len(r_300_mine)
p_50_300_mine = int(len_300_mine*0.5)
p_95_300_mine = int(len_300_mine*0.95)
p_99_300_mine = int(len_300_mine*0.99)
sum_mine_300 = 0
for x in r_300_mine:
    sum_mine_300 += x[1]
r_300_mine.sort(key=lambda x:x[1])
mine_50_300 = r_300_mine[p_50_300_mine][1]
mine_95_300 = r_300_mine[p_95_300_mine][1]
mine_99_300 = r_300_mine[p_99_300_mine][1]
#print(r_300[p_95][3])
#print(r_300[p_99][3])
line_300 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(300,sum_init_300/len_300_init,sum_mine_300/len_300_mine,init_50_300,mine_50_300,init_95_300,mine_95_300,init_99_300,mine_99_300)
print (line_300)

# 1000
len_1000_init = len(r_1000_init)
p_50_1000_init = int(len_1000_init*0.5)
p_95_1000_init = int(len_1000_init*0.95)
p_99_1000_init = int(len_1000_init*0.99)
sum_init_1000 = 0
for x in r_1000_init:
    sum_init_1000 += x[1]
#print(sum_init/len(r_1000))
#print(sum_mine/len(r_1000))
r_1000_init.sort(key=lambda x:x[1])
init_50_1000 = r_1000_init[p_50_1000_init][1]
init_95_1000 = r_1000_init[p_95_1000_init][1]
init_99_1000 = r_1000_init[p_99_1000_init][1]
#print(r_1000[p_95][2])
#print(r_1000[p_99][2])
len_1000_mine = len(r_1000_mine)
p_50_1000_mine = int(len_1000_mine*0.5)
p_95_1000_mine = int(len_1000_mine*0.95)
p_99_1000_mine = int(len_1000_mine*0.99)
sum_mine_1000 = 0
for x in r_1000_mine:
    sum_mine_1000 += x[1]
r_1000_mine.sort(key=lambda x:x[1])
mine_50_1000 = r_1000_mine[p_50_1000_mine][1]
mine_95_1000 = r_1000_mine[p_95_1000_mine][1]
mine_99_1000 = r_1000_mine[p_99_1000_mine][1]
#print(r_1000[p_95][3])
#print(r_1000[p_99][3])
line_1000 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(1000,sum_init_1000/len_1000_init,sum_mine_1000/len_1000_mine,init_50_1000,mine_50_1000,init_95_1000,mine_95_1000,init_99_1000,mine_99_1000)
print (line_1000)

# 2000
len_2000_init = len(r_2000_init)
p_50_2000_init = int(len_2000_init*0.5)
p_95_2000_init = int(len_2000_init*0.95)
p_99_2000_init = int(len_2000_init*0.99)
sum_init_2000 = 0
for x in r_2000_init:
    sum_init_2000 += x[1]
#print(sum_init/len(r_2000))
#print(sum_mine/len(r_2000))
r_2000_init.sort(key=lambda x:x[1])
init_50_2000 = r_2000_init[p_50_2000_init][1]
init_95_2000 = r_2000_init[p_95_2000_init][1]
init_99_2000 = r_2000_init[p_99_2000_init][1]
#print(r_2000[p_95][2])
#print(r_2000[p_99][2])
len_2000_mine = len(r_2000_mine)
p_50_2000_mine = int(len_2000_mine*0.5)
p_95_2000_mine = int(len_2000_mine*0.95)
p_99_2000_mine = int(len_2000_mine*0.99)
sum_mine_2000 = 0
for x in r_2000_mine:
    sum_mine_2000 += x[1]
r_2000_mine.sort(key=lambda x:x[1])
mine_50_2000 = r_2000_mine[p_50_2000_mine][1]
mine_95_2000 = r_2000_mine[p_95_2000_mine][1]
mine_99_2000 = r_2000_mine[p_99_2000_mine][1]
#print(r_2000[p_95][3])
#print(r_2000[p_99][3])
line_2000 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(2000,sum_init_2000/len_2000_init,sum_mine_2000/len_2000_mine,init_50_2000,mine_50_2000,init_95_2000,mine_95_2000,init_99_2000,mine_99_2000)
print (line_2000)

# 10000
len_10000_init = len(r_10000_init)
p_50_10000_init = int(len_10000_init*0.5)
p_95_10000_init = int(len_10000_init*0.95)
p_99_10000_init = int(len_10000_init*0.99)
sum_init_10000 = 0
for x in r_10000_init:
    sum_init_10000 += x[1]
#print(sum_init/len(r_10000))
#print(sum_mine/len(r_10000))
r_10000_init.sort(key=lambda x:x[1])
init_50_10000 = r_10000_init[p_50_10000_init][1]
init_95_10000 = r_10000_init[p_95_10000_init][1]
init_99_10000 = r_10000_init[p_99_10000_init][1]
#print(r_10000[p_95][2])
#print(r_10000[p_99][2])
len_10000_mine = len(r_10000_mine)
p_50_10000_mine = int(len_10000_mine*0.5)
p_95_10000_mine = int(len_10000_mine*0.95)
p_99_10000_mine = int(len_10000_mine*0.99)
sum_mine_10000 = 0
for x in r_10000_mine:
    sum_mine_10000 += x[1]
r_10000_mine.sort(key=lambda x:x[1])
mine_50_10000 = r_10000_mine[p_50_10000_mine][1]
mine_95_10000 = r_10000_mine[p_95_10000_mine][1]
mine_99_10000 = r_10000_mine[p_99_10000_mine][1]
#print(r_10000[p_95][3])
#print(r_10000[p_99][3])
line_10000 = "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"%(10000,sum_init_10000/len_10000_init,sum_mine_10000/len_10000_mine,init_50_10000,mine_50_10000,init_95_10000,mine_95_10000,init_99_10000,mine_99_10000)
print (line_10000)