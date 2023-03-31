import matplotlib.pyplot as plt
import csv
from matplotlib.font_manager import FontProperties
font = FontProperties(fname=r"C:\Windows\Fonts\simhei.ttf", size=14)  

x = []
y = []

# 打开example.txt 并且以读的方式打开
with open ('../core.txt','r') as file:
    #用csv去读文件 有关csv文件的格式请自行科谱
    #csv去读取文件并不只是读取以.csv结尾的文件，它只要满足是分隔数据格式就可以了，以逗号进行分隔的数据
    plots = csv.reader(file, delimiter=',')
    for row in plots:
        if((row[0][0]) != '#'):
            x.append(int(row[0]))
            y.append(float(row[1]))
        

plt.plot(x,y,label = 'Loaded from core.txt')

plt.xlabel('x')
plt.ylabel('y')

plt.title(u'测试从文件加载数据',FontProperties=font)

plt.legend()

plt.show()
