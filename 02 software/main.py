import tkinter as tk
from tkinter import ttk, filedialog
import serial
import serial.tools.list_ports
import datetime
import time
import csv
# 多线程 为了响应
import threading
from SI_Package.robot_chem_support_GUI import PathHandler, CSVHandler, SampleData
from SI_Package.robot_chem_support_GUI import SerialCommunication, Node2, Node3
from SI_Package.robot_chem_support_GUI import CommandParser, EmailSender
import logging
import os

# 创建 logs 目录（如果不存在）
if not os.path.exists("logs"):
    os.makedirs("logs")
# 配置日志记录
logging.basicConfig(
    level=logging.DEBUG,  # 设置最低日志级别为DEBUG，这样所有级别的日志都将被记录
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("logs/application.log"),  # 日志将被记录到文件中
        logging.StreamHandler()  # 同时输出到控制台
    ]
)

logger = logging.getLogger(__name__)

stop_thread = threading.Event()

def update_serial_ports():
    ports = serial.tools.list_ports.comports()
    available_ports = []
    for port in ports:
        available_ports.append(port.device)
    root_node_combobox['values'] = available_ports

def update_root_Node_light(is_success):
    global canvas_root_light, root_light
    if is_success:
        canvas_root_light.itemconfig(root_light, fill="green")
    else:
        canvas_root_light.itemconfig(root_light, fill="red")
def update_text(content,page=1):
    current_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    if page == 1:
        text.insert(tk.END, f"[{current_time}] {content}\n")
        text.see(tk.END)  # 自动滚动到最后一行
    elif page == 2:
        text2.insert(tk.END, f"[{current_time}] {content}\n")
        text2.see(tk.END)  # 自动滚动到最后一行
    elif page == 3:
        current_time2 = datetime.datetime.now().strftime('%H:%M:%S')
        text3.delete("1.0", tk.END)
        text3.insert(tk.END, f"[{current_time2}] {content}\n")
        text3.see(tk.END)  # 自动滚动到最后一行
def stop_select_cmd():
    global stop_thread
    stop_thread.set()
    update_text("Stopping the task...", page=2)
def open_cmd():
    selected_port = root_node_var.get()
    if selected_port:
        try:
            global Node_root, Node_root_status
            Node_root = SerialCommunication(port=selected_port, baud_rate=115200)
            if Node_root.open_serial_port():
                Node_root_status = True
                # buffer_content = Node_root.read_serial_buffer()
                # update_text(f"Serial buffer content:{buffer_content}")
                update_text(f"Root Node Com Open")
                update_root_Node_light(True)
            else:
                Node_root_status = False
                update_text(f"Failed to connect to {selected_port}")
                update_root_Node_light(False)
        except Exception as e:
            update_text(f"Error: {e}")
            return False
def close_cmd():
    global Node_root, Node_root_status
    if Node_root.close_serial_port():
        Node_root_status= False
        update_root_Node_light(False)
        update_text("root_node ports closed")
    else:
        update_text("failed close root_node ports")
def check_cmd(node_id,canvas_light, light):
    global Node_root,Node_root_status
    if Node_root_status:
        if node_id:
            if Node_root.node_call(node_id):
                canvas_light.itemconfig(light, fill="green")
                update_text(f"node {node_id} is in the RFnetwork")
            else:
                canvas_light.itemconfig(light, fill="red")
                update_text(f"node {node_id} is not in the RFnetwork")
    else:
        canvas_light.itemconfig(light, fill="grey")
        update_text("please open Node_Root port")
def select_all():
    global chk_var
    all_selected = all(var.get() == 1 for var in chk_var)
    if all_selected :
        for var in chk_var:
            var.set(0)
    else:
        for var in chk_var:
            var.set(1)
def select_row(row):
    global chk_var
    all_selected = all(chk_var[row * 6 + j].get() == 1 for j in range(6))
    if all_selected ==1 :
        for j in range(6):
            chk_var[row * 6 + j].set(0)
    else:
        for j in range(6):
            chk_var[row * 6 + j].set(1)
def select_column(col):
    global chk_var

    # 检查指定列的所有元素是否都等于1
    all_i_selected = all(chk_var[row * 6 + col].get() == 1 for row in range(5))

    if all_i_selected:
        # 如果该列所有元素都等于1，则将该列所有元素设置为0
        for row in range(5):
            chk_var[row * 6 + col].set(0)
    else:
        # 如果该列有任意元素不等于1，则将该列所有元素设置为1
        for row in range(5):
            chk_var[row * 6 + col].set(1)

def GUIBody_init():
    # 创建主窗口
    global root
    root = tk.Tk()
    root.title("PhemRobot 2024 GUI")
    root.iconbitmap('002.ico')

    # 创建菜单栏
    menu_bar = tk.Menu(root)
    root.config(menu=menu_bar)

    # 创建文件菜单
    file_menu = tk.Menu(menu_bar, tearoff=0)
    file_menu.add_command(label="打开")
    file_menu.add_command(label="保存")
    file_menu.add_separator()
    file_menu.add_command(label="退出", command=root.quit)
    menu_bar.add_cascade(label="File", menu=file_menu)

    # 创建编辑菜单
    edit_menu = tk.Menu(menu_bar, tearoff=0)
    edit_menu.add_command(label="剪切")
    edit_menu.add_command(label="复制")
    edit_menu.add_command(label="粘贴")
    menu_bar.add_cascade(label="Edit", menu=edit_menu)

    # 创建帮助菜单
    help_menu = tk.Menu(menu_bar, tearoff=0)
    help_menu.add_command(label="关于")
    menu_bar.add_cascade(label="Help", menu=help_menu)

    global notebook
    # 创建Notebook对象
    notebook = ttk.Notebook(root)
    notebook.pack(expand=True, fill='both')
def GUIPage1():
    global Node_root_status
    Node_root_status = False
    # page1-Frame
    page1 = ttk.Frame(notebook)
    notebook.add(page1, text='Setting')

    frame_csv = ttk.Frame(page1)
    frame_csv.grid(row=0, column=0, columnspan=5, padx=5, pady=5, sticky="w")

    separator1 = ttk.Separator(page1, orient='horizontal')
    separator1.grid(row=1, column=0, columnspan=5, sticky="ew", pady=10)


    frame_rootNode = ttk.Frame(page1)
    frame_rootNode.grid(row=2, column=0, columnspan=5, padx=5, pady=5, sticky="w")

    separator2 = ttk.Separator(page1, orient='horizontal')
    separator2.grid(row=3, column=0, columnspan=5, sticky="ew", pady=10)

    frame_subNode = ttk.Frame(page1)
    frame_subNode.grid(row=4, column=0, columnspan=5, padx=5, pady=5, sticky="w")

    separator3 = ttk.Separator(page1, orient='horizontal')
    separator3.grid(row=5, column=0, columnspan=5, sticky="ew", pady=10)

    frame_text = ttk.Frame(page1)
    frame_text.grid(row=6, column=0, columnspan=5, padx=5, pady=5, sticky="w")

    # # frame_csv
    node_label = ttk.Label(frame_csv, text=f"Postion.CSV")
    node_label.grid(row=0, column=0, padx=5, pady=5)
                # # # # column01
    file_button0 = ttk.Button(frame_csv, text="Select", command=lambda: select_file(file_entry1,update_position_csv))
    file_button0.grid(row=0, column=1, padx=5, pady=5)
                # # # # column02
    global file_entry1
    file_entry1 = ttk.Entry(frame_csv, width=50)
    file_entry1.grid(row=0, column=2, padx=5, pady=5)
            # # # row01
                # # # # column00
    node_label2 = ttk.Label(frame_csv, text=f"Sample01.CSV")
    node_label2.grid(row=1, column=0, padx=5, pady=5)
                # # # # column01
    file_button2 = ttk.Button(frame_csv, text="Select", command=lambda: select_file(file_entry2,update_sample1_csv))
    file_button2.grid(row=1, column=1, padx=5, pady=5)
                # # # # column02
    global file_entry2
    file_entry2 = ttk.Entry(frame_csv, width=50)
    file_entry2.grid(row=1, column=2, padx=5, pady=5)

         # # # row02
                # # # # column00
    node_label3 = ttk.Label(frame_csv, text=f"Sample02.CSV")
    node_label3.grid(row=2, column=0, padx=5, pady=5)
                # # # # column01
    file_button3 = ttk.Button(frame_csv, text="Select", command=lambda: select_file(file_entry3,update_sample2_csv))
    file_button3.grid(row=2, column=1, padx=5, pady=5)
                # # # # column02
    global file_entry3
    file_entry3 = ttk.Entry(frame_csv, width=50)
    file_entry3.grid(row=2, column=2, padx=5, pady=5)

   # # frame_rootNode
    global canvas_root_light,root_light
    canvas_root_light = tk.Canvas(frame_rootNode, width=20, height=20)
    canvas_root_light.grid(row=0, column=0, padx=5, pady=5)
    root_light = canvas_root_light.create_oval(5, 5, 20, 20, fill="grey")

    root_node_label = ttk.Label(frame_rootNode, text="Node_Root")
    root_node_label.grid(row=0, column=1, padx=5, pady=5, sticky="w")

        # # row0 col2 添加下拉菜单
    global root_node_var,root_node_combobox
    root_node_var = tk.StringVar()  # 选择变量存储器
    root_node_combobox = ttk.Combobox(frame_rootNode, textvariable=root_node_var, width=8)  # 下拉框01
    root_node_combobox.grid(row=0, column=2, padx=5, pady=5)  # 下拉选框01.位置
    update_serial_ports()  # 更新串口列表给下拉框01
        # # row0 col3 添加Open按钮
    root_node_button = ttk.Button(frame_rootNode, text="Open",command = open_cmd)
    root_node_button.grid(row=0, column=3, padx=5, pady=5)

        # # row0 col4 添加Close按钮
    root_node_button2 = ttk.Button(frame_rootNode, text="Close", command = close_cmd)
    root_node_button2.grid(row=0, column=4, padx=5, pady=5)

    # # frame_subNode
    row_of_motion_node = 0
    global canvas_motion_node_light, motion_node_light, motion_node_var
    canvas_motion_node_light = tk.Canvas(frame_subNode, width=20, height=20)
    canvas_motion_node_light.grid(row=row_of_motion_node, column=0, padx=5, pady=5)
    motion_node_light = canvas_motion_node_light.create_oval(5, 5, 20, 20, fill="grey")
    motion_node_label = ttk.Label(frame_subNode, text=f"Motion Node")
    motion_node_label.grid(row=row_of_motion_node, column=1, padx=5, pady=5, sticky="w")

    motion_node_var = tk.StringVar()
    motion_node_combobox = ttk.Combobox(frame_subNode, textvariable=motion_node_var, values=list(range(1, 11)), width=8)
    motion_node_combobox.grid(row=row_of_motion_node, column=2, padx=5, pady=5)
    motion_node_combobox.current(1)

    node_button = ttk.Button(frame_subNode, text="Check", command = lambda: check_cmd(int(motion_node_var.get()),canvas_motion_node_light,motion_node_light))
    node_button.grid(row=row_of_motion_node, column=3, padx=5, pady=5)

    sample_node01 = 1
    global canvas_sample_node01_light,sample_node01_light, sample_node01_var
    canvas_sample_node01_light = tk.Canvas(frame_subNode, width=20, height=20)
    canvas_sample_node01_light.grid(row=sample_node01, column=0, padx=5, pady=5)
    sample_node01_light = canvas_sample_node01_light.create_oval(5, 5, 20, 20, fill="grey")
    sample_node01_label = ttk.Label(frame_subNode, text=f"Sample Node 01")
    sample_node01_label.grid(row=sample_node01, column=1, padx=5, pady=5, sticky="w")

    sample_node01_var = tk.StringVar()
    sample_node01_combobox = ttk.Combobox(frame_subNode, textvariable=sample_node01_var, values=list(range(1, 11)), width=8)
    sample_node01_combobox.grid(row=sample_node01, column=2, padx=5, pady=5)
    sample_node01_combobox.current(2)

    node_button = ttk.Button(frame_subNode, text="Check", command = lambda: check_cmd(int(sample_node01_var.get()),canvas_sample_node01_light,sample_node01_light))
    node_button.grid(row=sample_node01, column=3, padx=5, pady=5)

    sample_node02 = 2
    # column=0 light
    global canvas_sample_node02_light, sample_node02_light, sample_node02_var
    canvas_sample_node02_light = tk.Canvas(frame_subNode, width=20, height=20)
    canvas_sample_node02_light.grid(row=sample_node02, column=0, padx=5, pady=5)
    sample_node02_light = canvas_sample_node02_light.create_oval(5, 5, 20, 20, fill="grey")

    sample_node02_label = ttk.Label(frame_subNode, text=f"Sample Node 02")
    sample_node02_label.grid(row=sample_node02, column=1, padx=5, pady=5, sticky="w")

    sample_node02_var = tk.StringVar()
    sample_node02_combobox = ttk.Combobox(frame_subNode, textvariable=sample_node02_var, values=list(range(1, 11)),
                                          width=8)
    sample_node02_combobox.grid(row=sample_node02, column=2, padx=5, pady=5)
    sample_node02_combobox.current(3)

    node_button = ttk.Button(frame_subNode, text="Check",
                             command=lambda: check_cmd(int(sample_node02_var.get()), canvas_sample_node02_light,
                                                       sample_node02_light))
    node_button.grid(row=sample_node02, column=3, padx=5, pady=5)

    global text
    text = tk.Text(frame_text, wrap="word", width=80, height=10)
    text.grid(row=0, column=0, sticky="ns")
    scrollbar = ttk.Scrollbar(frame_text, orient="vertical", command=text.yview)
    scrollbar.grid(row=0, column=1, sticky="ns")
    text.config(yscrollcommand=scrollbar.set)
def GUIPage2():
    # 创建第二页
    global page2,p2_cs_frame, chk_var, chk, btn,check_lifting,text2,text3,check_circle,text2
    p2_cs_frame, chk_var, chk, btn = [], [], [], []
    # page frame structure
    page2 = ttk.Frame(notebook)
    notebook.add(page2, text='Sample')

    frame_toolbar = ttk.Frame(page2)
    frame_toolbar.grid(row=1, column=0, padx=5, pady=5, sticky="w")

    frame_top = ttk.Frame(page2)
    frame_top.grid(row=0, column=0, padx=5, pady=5, sticky="w")

    frame_main = ttk.Frame(page2)
    frame_main.grid(row=2, column=0, padx=5, pady=5, sticky="w")

    frame_bottom = ttk.Frame(page2)
    frame_bottom.grid(row=3, column=0, padx=5, pady=5, sticky="w")

    #detail
    text3 = tk.Text(frame_top, wrap="word", width=80, height=1)
    text3.grid(row=0, column=0, sticky="ns")

    ttk.Button(frame_main, text="Select All", command=select_all).grid(row=0, column=0, padx=5, pady=5)
    # ttk.Button(frame_main, text="Deselect All", command=deselect_all).grid(row=0, column=1, padx=5, pady=5)

    check_lifting = tk.BooleanVar()
    checkbutton = tk.Checkbutton(frame_toolbar, text="Open Lifting", variable=check_lifting)
    checkbutton.grid(row=0, column=0, padx=10, pady=10)

    check_circle = tk.BooleanVar()
    checkbutton2 = tk.Checkbutton(frame_toolbar, text="Open runCircle", variable=check_circle)
    checkbutton2.grid(row=0, column=1, padx=10, pady=10)

    for j in range(6):
        ttk.Button(frame_main, text=f"Select",
                   command=lambda j=j: select_column(j)).grid(row=0,column=j+2,padx=5,pady=5, sticky="e")

        # ttk.Button(frame_main, text=f"Deselect",
        #            command=lambda j=j: deselect_column(j)).grid(row=1,column=j+2,padx=5,pady=5, sticky="e")

        # 创建并放置列标题
    for i in range(5):
        ttk.Button(frame_main, text=f"Select",
                   command=lambda i=i: select_row(i)).grid(row=i+2,column=0,padx=5, pady=5)

        # ttk.Button(frame_main, text=f"Deselect",
        #            command=lambda i=i: deselect_row(i)).grid(row=i+2,column=1,padx=5,pady=5)

    index = 0
    for i in range(5):
        for j in range(6):
            p2_cs_frame.append(ttk.Frame(frame_main))
            p2_cs_frame[index].grid(row=i + 2, column=j + 2, padx=10, pady=20, sticky="w")

            chk_var.append(tk.IntVar(value=0))
            chk.append(ttk.Checkbutton(p2_cs_frame[index], variable=chk_var[index], text="", onvalue=1, offvalue=0))
            chk[index].pack(side="left")

            btn.append(ttk.Button(p2_cs_frame[index], text=f"Sample {index+1}"))
            btn[index].pack(side="left")
            index += 1

    # run_botton_frame = ttk.Frame(page2)
    # run_botton_frame.grid(row=2, column=0, padx=5, pady=5, sticky="w")

    run_select_button = ttk.Button(frame_bottom, text="Run", command=run_select_cmd)
    run_select_button.grid(row=0, column=0, padx=5, pady=5, sticky="w")

    run_select_button = ttk.Button(frame_bottom, text="Stop", command=stop_select_cmd)
    run_select_button.grid(row=0, column=1, padx=5, pady=5, sticky="w")
def GUIPage3():
    page3 = ttk.Frame(notebook)
    notebook.add(page3, text='logging')

    frame_text = ttk.Frame(page3)
    frame_text.grid(row=0, column=0, padx=5, pady=5, sticky="w")
    global text2
    text2 = tk.Text(frame_text, wrap="word", width=60, height=28)
    text2.grid(row=0, column=0, sticky="ns")
    scrollbar2 = ttk.Scrollbar(frame_text, orient="vertical", command=text2.yview)
    scrollbar2.grid(row=0, column=1, sticky="ns")
    text2.config(yscrollcommand=scrollbar2.set)
def GUIPage4():
    page4 = ttk.Frame(notebook)
    notebook.add(page4, text='Position Data')

    global text_position
    text_position = tk.Text(page4, wrap="none", width=80, height=28)
    text_position.grid(row=0, column=0, sticky="nsew")

    scrollbar_y = ttk.Scrollbar(page4, orient="vertical", command=text_position.yview)
    scrollbar_y.grid(row=0, column=1, sticky="ns")
    scrollbar_x = ttk.Scrollbar(page4, orient="horizontal", command=text_position.xview)
    scrollbar_x.grid(row=1, column=0, sticky="ew")

    text_position.config(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)
def GUIPage5():
    page5 = ttk.Frame(notebook)
    notebook.add(page5, text='Sample01 Data')

    global text_sample1
    text_sample1 = tk.Text(page5, wrap="none", width=80, height=28)
    text_sample1.grid(row=0, column=0, sticky="nsew")

    scrollbar_y = ttk.Scrollbar(page5, orient="vertical", command=text_sample1.yview)
    scrollbar_y.grid(row=0, column=1, sticky="ns")
    scrollbar_x = ttk.Scrollbar(page5, orient="horizontal", command=text_sample1.xview)
    scrollbar_x.grid(row=1, column=0, sticky="ew")

    text_sample1.config(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)
def GUIPage6():
    page6 = ttk.Frame(notebook)
    notebook.add(page6, text='Sample02 Data')

    global text_sample2
    text_sample2 = tk.Text(page6, wrap="none", width=80, height=28)
    text_sample2.grid(row=0, column=0, sticky="nsew")

    scrollbar_y = ttk.Scrollbar(page6, orient="vertical", command=text_sample2.yview)
    scrollbar_y.grid(row=0, column=1, sticky="ns")
    scrollbar_x = ttk.Scrollbar(page6, orient="horizontal", command=text_sample2.xview)
    scrollbar_x.grid(row=1, column=0, sticky="ew")

    text_sample2.config(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)

def set_fixed_width_font(widget):
    widget.config(font=("Courier", 10))  # Set to Courier, which is monospaced

def update_position_csv():
    filename = file_entry1.get()  # Position CSV file
    if filename:  # Ensure a file is selected
        try:
            with open(filename, 'r', newline='') as file:
                text_position.delete("1.0", tk.END)
                reader = csv.reader(file)
                for row in reader:
                    formatted_row = ''.join(f"{item:<15}" for item in row)  # Left align each item with padding
                    text_position.insert(tk.END, formatted_row + '\n')
        except Exception as e:
            update_text(f"Error reading Position CSV: {str(e)}")

def update_sample1_csv():
    filename = file_entry2.get()  # Sample1 CSV file
    if filename:  # Ensure a file is selected
        try:
            with open(filename, 'r', newline='') as file:
                text_sample1.delete("1.0", tk.END)
                reader = csv.reader(file)
                for row in reader:
                    formatted_row = ''.join(f"{item:<15}" for item in row)  # Left align each item with padding
                    text_sample1.insert(tk.END, formatted_row + '\n')
        except Exception as e:
            update_text(f"Error reading Sample01 CSV: {str(e)}")

def update_sample2_csv():
    filename = file_entry3.get()  # Sample2 CSV file
    if filename:  # Ensure a file is selected
        try:
            with open(filename, 'r', newline='') as file:
                text_sample2.delete("1.0", tk.END)
                reader = csv.reader(file)
                for row in reader:
                    formatted_row = ''.join(f"{item:<15}" for item in row)  # Left align each item with padding
                    text_sample2.insert(tk.END, formatted_row + '\n')
        except Exception as e:
            update_text(f"Error reading Sample02 CSV: {str(e)}")

def check_box():
    for i in range(30):
        if chk[i]:
            update_text(f"chk: {chk[i]}",page=2)
            update_text(f"number{i} is selected",page=2)
        else:
            update_text(f"number{i} not!",page=2)
def select_file(entry, update_function):
    file_path = filedialog.askopenfilename()
    if file_path:  # 确保用户选择了一个文件
        entry.delete(0, tk.END)
        entry.insert(0, file_path)
        update_function()  # 选择文件后更新内容
def read_csv(entry_file,index,column_name):
    filename = entry_file.get()
    try:
        with open(filename, 'r', newline='') as file:
            reader = csv.reader(file)
            csv_data = list(reader)
            if index < len(csv_data):
                if column_name in csv_data[0]:
                    column_index = csv_data[0].index(column_name)
                    value = csv_data[index][column_index]
                    update_text(f"Value at index {index} in column '{column_name}': {value}",page=2)
                    return value
                else:
                    update_text(f"Column '{column_name}' not found in CSV file.",page=2)
            else:
                update_text(f"Index {index} is out of range.",page=2)

    except FileNotFoundError:
        update_text("File not found.",page=2)
    except ValueError:
        update_text("Invalid index.",page=2)
    except Exception as e:
        update_text(f"Error: {str(e)}",page=2)
def run_select_cmd():
    global stop_thread
    stop_thread.clear()
    # 创建新线程来执行长时间任务
    threading.Thread(target=run_select_task).start()
def run_select_task():
    update_text("This process may take a while (minutes to hours)", page=2)
    update_text("Don't click and wait until finishe", page=2)
    global Node_root, file_entry1, file_entry2, motion_node_var, sample_node01_var,check_lifting
    motion_node_id = int(motion_node_var.get())
    sample_node01_id = int(sample_node01_var.get())
    if motion_node_id:
        update_text("Start Home Position", page=2)
        Node_root.exe_cmd(f"${motion_node_id}c2")
        time.sleep(1)
        update_text("Home Position Success", page=2)
        update_text("Start Making Sample Process. Please don't operate GUI page", page=2)
        for i in range(1,31):
            if stop_thread.is_set():
                update_text("Task has been stopped by user.", page=2)
                break
            progress = "#" * (i) + "-" * (29 - i)
            update_text(f"progress {progress}", page=3)
            if chk[i-1].instate(['selected']):
                # 向上移动到endswitch 之后向下移动 dispenser_up
                # Node_root.exe_cmd(f"${motion_node_id}c7")
                update_text(f"Start sample {i}/30", page=2)
                if motion_node_id:
                    temp_status = check_lifting.get()
                    temp_status2 = check_circle.get()
                    p_x = read_csv(file_entry1, i, "x")
                    p_y = read_csv(file_entry1, i, "y")
                    p_z = read_csv(file_entry1, i, "z")
                    Node_root.exe_cmd(f"${motion_node_id}c4x{p_x}y{p_y}")
                    if temp_status:
                        Node_root.exe_cmd(f"${motion_node_id}c3z{p_z}")
                    update_text(f"Reach Position {i}", page=2)
                if sample_node01_id:
                    s_x = read_csv(file_entry2, i, "x")
                    s_y = read_csv(file_entry2, i, "y")
                    s_z = read_csv(file_entry2, i, "z")
                    s_x_gradient = read_csv(file_entry2, 1, "x_g")
                    s_y_gradient = read_csv(file_entry2, 1, "y_g")
                    s_z_gradient = read_csv(file_entry2, 1, "z_g")
                    s_x_step = round(float(s_x) / float(s_x_gradient), 0)
                    s_y_step = round(float(s_y) / float(s_y_gradient), 0)
                    s_z_step = round(float(s_z) / float(s_z_gradient), 0)
                    Node_root.exe_cmd(f"${sample_node01_id}c2x{s_x_step}y{s_y_step}z{s_z_step}")
                    temp_status = check_lifting.get()
                    if temp_status2:
                        # start dispenser_up
                        Node_root.exe_cmd(f"${motion_node_id}c5r500n1")
                    if temp_status:
                        # start dispenser_up
                        Node_root.exe_cmd(f"${motion_node_id}c3z0")
            update_text(f"Task Completed {i + 1}/30", page=2)

        if temp_status:
            Node_root.exe_cmd(f"${motion_node_id}c3z0")

        Node_root.exe_cmd(f"${motion_node_id}c4x{5500}y{0}")
            # else:
            #     update_text(f"Sample {i + 1} is not selected", page=2)

        # 设置发件人和收件人信息
        sender_email = "cn_mengzy@outlook.com"
        receiver_email = "cn_mengzy@163.com"

        # 设置邮件内容
        message = MIMEText("Task Completed.")
        message["Subject"] = "Experiment Finished"

        # 设置服务器信息
        smtp_server = "smtp-mail.outlook.com"
        port = 587

        # 创建连接
        with smtplib.SMTP(smtp_server, port) as server:
            # 登录服务器
            server.starttls()
            server.login(sender_email, "Meng1994!")

            # 发送邮件
            server.sendmail(sender_email, receiver_email, message.as_string())
if __name__ == "__main__":
    # 创建主体
    GUIBody_init()
    # 配置page01
    GUIPage1()
    GUIPage2()
    GUIPage3()
    GUIPage4()
    GUIPage5()
    GUIPage6()
    # 运行主循环
    root.mainloop()