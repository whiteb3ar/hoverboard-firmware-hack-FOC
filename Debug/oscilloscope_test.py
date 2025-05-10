import serial
import struct
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import defaultdict

data = defaultdict(lambda: {'time': [], 'value': []})


fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(10, 8))
plt.tight_layout(pad=3.0)

lines = {}

data[1]['time'].append(0)
data[1]['value'].append(1)
data[1]['time'].append(1)
data[1]['value'].append(2)
data[1]['time'].append(3)
data[1]['value'].append(0)

data[2]['time'].append(0)
data[2]['value'].append(2)
data[2]['time'].append(1)
data[2]['value'].append(0)
data[2]['time'].append(3)
data[2]['value'].append(1)

def init_plots():
    for ax in axes:
        ax.clear()
        ax.grid(True)
    return []

def update(frame):
    for i, (type_val, values) in enumerate(data.items()):
        if i >= len(axes):
            break
        
        if values['time'] and values['value']:
            axes[i].clear()
            axes[i].plot(values['time'], values['value'], label=f'Type {type_val}')
            axes[i].set_title(f'Type {type_val}')
            axes[i].set_xlabel('Time')
            axes[i].set_ylabel('Value')
            axes[i].grid(True)
            axes[i].legend()

    plt.tight_layout(pad=3.0)

init_plots()
update(0)

#ani = FuncAnimation(fig, update, init_func=init_plots, interval=100)

plt.show()