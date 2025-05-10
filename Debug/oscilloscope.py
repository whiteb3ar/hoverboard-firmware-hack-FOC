import serial
import struct
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import defaultdict

# Настройка UART (замените параметры на свои)
ser = serial.Serial(
    port='COM3',      # Замените на ваш порт (например, '/dev/ttyUSB0' для Linux)
    baudrate=115200,
    timeout=1
)

# Структура для хранения данных
data = defaultdict(lambda: {'time': [], 'value': []})

# Функция для разбора входящих данных
def parse_data(packet):
    try:
        # Предполагаем, что данные приходят в виде 6 байт: 2 байта time, 2 байта type, 2 байта value
        if len(packet) == 6:
            time, type_val, value = struct.unpack('<HHH', packet)  # little-endian, 3 uint16_t
            return time, type_val, value
    except Exception as e:
        print(f"Ошибка разбора данных: {e}")
    return None

# Создаем фигуру с субграфиками
fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(10, 8))  # 3 субграфика (можно увеличить)
plt.tight_layout(pad=3.0)

# Словарь для хранения линий графиков (чтобы их обновлять)
lines = {}

# Инициализация графиков
def init_plots():
    for ax in axes:
        ax.clear()
        ax.grid(True)
    return []

# Функция для обновления графиков
def update(frame):
    # Чтение данных из UART
    while ser.in_waiting >= 6:
        packet = ser.read(6)
        parsed = parse_data(packet)
        if parsed:
            time, type_val, value = parsed
            data[type_val]['time'].append(time)
            data[type_val]['value'].append(value)
    
    # Отрисовываем данные для каждого типа в своем субграфике
    for i, (type_val, values) in enumerate(data.items()):
        if i >= len(axes):
            break  # Не больше субграфиков, чем задано
        
        if values['time'] and values['value']:
            axes[i].clear()
            axes[i].plot(values['time'], values['value'], label=f'Type {type_val}')
            axes[i].set_title(f'Type {type_val}')
            axes[i].set_xlabel('Time')
            axes[i].set_ylabel('Value')
            axes[i].grid(True)
            axes[i].legend()

    plt.tight_layout(pad=3.0)

# Настройка анимации
ani = FuncAnimation(fig, update, init_func=init_plots, interval=100)

plt.show()

# Закрытие UART при завершении
ser.close()