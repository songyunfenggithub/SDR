import matplotlib.pyplot as plt
import csv
from matplotlib.font_manager import FontProperties
font = FontProperties(fname=r"C:\Windows\Fonts\simhei.ttf", size=14)  

x = []
y = []

fr = open('../core.txt')
for line in fr.readlines():
    line = line.strip()
    if line != '' and line[0] != '#':    #去除空白行 和 注释
        row = line.split(',')
        x.append(int(row[0]))
        y.append(float(row[1]))
        
plt.plot(x,y,label = 'Loaded from core.txt')

plt.xlabel('x')
plt.ylabel('y')

plt.title(u'测试从文件加载数据',FontProperties=font)

plt.legend()

plt.show()
