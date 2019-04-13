import matplotlib.pyplot as plt
import matplotlib
import numpy as np

matplotlib.rcParams['xtick.direction'] = 'in'
matplotlib.rcParams['ytick.direction'] = 'in'

matplotlib.rcParams['xtick.major.width'] = 4
matplotlib.rcParams['xtick.major.size'] = 8

matplotlib.rcParams['ytick.major.width'] = 4
matplotlib.rcParams['ytick.major.size'] = 8

matplotlib.rcParams['xtick.labelsize'] = 20
matplotlib.rcParams['ytick.labelsize'] = 20

matplotlib.rcParams['ytick.right'] = True
matplotlib.rcParams['xtick.top'] = True

matplotlib.rcParams['axes.linewidth'] = 4
matplotlib.rcParams['axes.labelsize'] = 20
matplotlib.rcParams['axes.labelcolor'] = 'black'
matplotlib.rcParams['axes.facecolor'] = 'white'
matplotlib.rcParams['axes.edgecolor'] = 'black'

matplotlib.rcParams['grid.color'] = 'black'
matplotlib.rcParams['grid.linestyle'] = 'dotted'
matplotlib.rcParams['grid.linewidth'] = 2

matplotlib.rcParams['patch.force_edgecolor'] = True
matplotlib.rcParams['patch.edgecolor'] = 'k'

matplotlib.rcParams['legend.fancybox']= False
matplotlib.rcParams['legend.frameon']= False
matplotlib.rcParams['font.style'] = 'oblique'
matplotlib.rcParams['figure.subplot.left'] = 0.15
matplotlib.rcParams['figure.subplot.bottom'] = 0.25
matplotlib.rcParams['figure.figsize'] = [12,8]

matplotlib.rcParams['legend.loc'] = 'upper left'
matplotlib.rcParams['legend.fontsize'] = 24

color = ['m','y','salmon','peru','k','b','g','r','c']
shape = ['D','o','x','+','*','v','^','s','p']

fig_index = 0

msg = [8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608]
reg = [21.5, 16, 11.6, 10.2, 14.8, 11.3, 9.8, 14.9, 10.1, 10.2, 15.1, 12.5, 14.4, 15.5, 24.3, 25.6, 40.5, 67.2, 110.8, 194.2, 332.7]
cpy = [0,  0,  0,  0,   0,   0,   0,    0,    0,  0.2,  0.1,   0.3,   2.4,   4.7,   11.7,     21,   43.4,   191.5,   524.5,  1203.7,  2738.3]

def plot_bar():
    index = np.arange(len(msg))
    plt.bar(index-0.15, reg, width=0.3, label='Memreg')
    plt.bar(index+0.15, cpy, width=0.3, label='Memcpy')
    
    plt.grid(axis='y')
    plt.legend()
    plt.xlabel("Data Size (Bytes)")
    plt.xticks(index, msg, rotation=45)
    plt.ylabel("Time (us)")
    plt.semilogy()
    plt.show()

if __name__ == "__main__":
    plot_bar()
