import os
import re
import sys
import time
import serial
import logging
import datetime
import pandas as pd
import smtplib
from email.mime.text import MIMEText

class PathHandler:
    def __init__(self, root_path):
        self.root_path = root_path

    def read_csv(self, filename):
        """读取指定路径下的 CSV 文件"""
        csv_file_path = os.path.join(self.root_path, "SI_Info", filename)
        if os.path.exists(csv_file_path):
            try:
                df = pd.read_csv(csv_file_path)
                return df
            except Exception as e:
                logging.error("Error reading CSV file:", e)
                return None
        else:
            logging.info("CSV file does not exist.")
            return None

    def create_log_file(self):
        """创建当天日期的日志文件"""
        log_folder = os.path.join(self.root_path, "log")
        if not os.path.exists(log_folder):
            os.makedirs(log_folder)
        current_datetime = datetime.datetime.now()
        formatted_datetime = current_datetime.strftime("%Y-%m-%d_%H-%M")
        log_filename = os.path.join(log_folder, f"{formatted_datetime}_log.txt")
        logging.basicConfig(filename=log_filename, level=logging.DEBUG,
                            format="%(asctime)s - %(levelname)s - %(message)s")

class CSVHandler:
    def __init__(self, df):
        self.df = df

    def read_csv(self, filePath):
        """读取指定路径下的 CSV 文件"""
        if os.path.exists(csv_file_path):
            try:
                df = pd.read_csv(csv_file_path)
                return df
            except Exception as e:
                logging.error("Error reading CSV file:", e)
                return None
        else:
            logging.info("CSV file does not exist.")
            return None

    def get_data2(self, index, column_name):
        """获取指定列和索引的数据"""
        try:
            data = self.df.at[index, column_name]
            return int(data)
        except Exception as e:
            logging.error("Error getting data:", e)
            return None

    def get_data(self, index, column_name, default_value=0):
        """获取指定列和索引的数据"""
        try:
            data = self.df.at[index, column_name]
            if pd.isna(data):
                return default_value
            else:
                return int(data)
        except Exception as e:
            logging.info("Error getting data:", e)
            return None

    def get_values(self, index, *column_names):
        """获取指定行和列的值，并返回以列名和数值组成的字符串
            1个index后面无限个column_name
        """

        values = []
        for column_name in column_names:
            value = self.get_data(index,column_name)
            if value is not None:
                column_name_without_digits = re.sub(r'\d+', '', column_name)
                values.append(f"{column_name_without_digits}{value}")
        return " ".join(values).replace(" ", "")

class SampleData:
    def __init__(self, index, sample_csv, coordinates_csv):
        self.index = index
        self.csv_sample = sample_csv
        self.csv_vial = coordinates_csv
        self.sample = self._get_sample()
        self.coordinates = self._get_coordinates()

    def _get_sample(self):
        """通过索引获取样本数据"""
        samples = {}
        samples["x"] = self.csv_sample.get_values(self.index, "x") + "y0z0"

        # 获取 y 值并根据要求添加后缀
        samples["y"] = "x0" + self.csv_sample.get_values(self.index, "y") + "z0"

        # 获取 z 值并根据要求添加后缀
        samples["z"] = "x0y0" + self.csv_sample.get_values(self.index, "z")
        return samples

    def _get_coordinates(self):
        """通过索引获取坐标数据"""
        coordinates = {}
        for i in range(1,5):
            x_column = f"x{i}"
            y_column = f"y{i}"
            coordinates_data = self.csv_vial.get_values(self.index, x_column, y_column)
            coordinates[f"{i}"] = coordinates_data
        return coordinates

class SerialCommunication:
    def __init__(self, port, baud_rate, timeout=3):
        self.port = port
        self.baud_rate = baud_rate
        self.timeout = timeout
        self.ser = None

    def open_serial_port(self):
        try:
            self.ser = serial.Serial(self.port, self.baud_rate, timeout=self.timeout)
            logging.info(f"Open port {self.port} success!")
            # time.sleep(2)
            return True
        except serial.SerialException as e:
            logging.error(f"Open port {self.port} failed, error code: {e}")
            return False

    def close_serial_port(self):
        """Closes the serial port if it is open."""
        if self.ser is not None:
            try:
                self.ser.close()
                logging.info("Serial port closed.")
                return True
            except serial.SerialException as e:
                logging.error(f"Error closing serial port: {e}")
                return False

    def read_serial_buffer(self):
        buffer_content = b""
        # time_stop = time.time()+5
        # while time.time()<time_stop:
        while self.ser.in_waiting > 0:
            buffer_content += self.ser.read(self.ser.in_waiting)
            logging.info(f"def Read serial buffer: {buffer_content}.")
            return buffer_content.decode().strip()

    def tx_cmd(self, command):
        if self.ser is not None:
            self.ser.write(command.encode())
            time.sleep(1)
            logging.info(f'def tx_cmd : send {command}')
            return True
        else:
            logging.debug(f"def tx_cmd : didn't send {command}")
            return False

    def rx_ok(self, timeout=30):
        time_stop = time.time() + timeout
        while time.time() < time_stop:
            response = self.ser.readline().decode().strip()
            if response is None:
                pass
            else:
                if "ok" in response.lower():
                    return True
                else:
                    response = self.ser.readline().decode().strip()
                    if "ok" in response.lower():
                        return True
                    else:
                        return False
            time.sleep(0.5)  # 减少 sleep 时间以提高检查频率
        logging.warning("rx_ok: Timed out without receiving 'ok'.")
        return False

    def tx_cmd_rx_ok(self, command):
        self.tx_cmd(command)
        if self.rx_ok():
            return True
        else:
            return False

    # def rx_mark(self, timeout=300, completion_char='!'):
    #     time_stop = time.time() + timeout
    #     while self.ser.in_waiting > 0:
    #         while time.time() < time_stop:
    #             response = self.ser.readline().decode().strip()
    #             logging.info(f"def rx_mark : receive{response}")
    #             if completion_char in response:
    #                 return True
    #     logging.debug(f"def rx_mark : didn't receive ! ")
    #     return False


    def exe_cmd(self, command,  timeout=300, call_time=10, interval=30):
        time_stop = time.time() + timeout
        node_id = CommandParser.parse_node_id(command)
        self.ser.reset_input_buffer()
        while time.time() < time_stop:
            send_cmd = self.tx_cmd_rx_ok(command)
            if send_cmd:
                self.ser.reset_input_buffer()
                msg=self.node_call(node_id,call_time=300,interval=3)
                # msg = self.rx_mark()#等待rx_mark(!) timeout=600s
                if msg:
                    return True
                elif self.node_call(node_id,call_time=360,interval=5):# 如果时间超过,退出系统
                    logging.info("exe_cmd use read_serial_buffer next line:")
                    self.read_serial_buffer()
                    return True
                # else:
                #     # 如果时间未超过，但是仍未收到执行完毕消息，则主动询问节点状态
                #     if self.node_call(node_id, call_time, interval):#会循环call_time次,每次间隔interval 300s
                #         return True
                else:
                    return False # 超过尝试次数，跳出循环
            time.sleep(5)
        return False

    def node_call(self, node_id, call_time=1, interval=1, timeout=3):
        """
        node_call给相应的node发送${node_id}c1
        如果回应了ok说明node是空闲状态，返回True.
        如果尝试了call_time次，仍然没收到ok的回应，则返回False
        call_time是呼叫次数，默认5次；
        interval是每次呼叫的间隔，默认30s
        """
        for i in range(call_time):
            self.tx_cmd(f"${node_id}c1")
            if self.rx_ok(timeout=timeout):
                return True
            else:
                time.sleep(interval)
        return False

class NodeCommandGenerator:
    def __init__(self, node_id):
        self.node_id = node_id
        self.explanation_command = f"${self.node_id}c0"
        self.ok_command = f"${self.node_id}c1"

    def generate_command(self, command_type, value):
        """生成命令"""
        if isinstance(value, str):
            command = f"${self.node_id}c{command_type}{value}"
        else:
            command = f"${self.node_id}c{command_type}{value}"
        return command

class Node2(NodeCommandGenerator):
    def __init__(self, node_id):
        super().__init__(node_id)
    def init_machine(self):
        """机器初始化命令"""
        return self.generate_command(2, "")

    def move_to_target(self, target_position):
        """走到目标位置命令"""
        return self.generate_command(4, target_position)

    def lift_needle(self):
        """抬升needle命令"""
        return self.generate_command(7, "")

    def lower_needle(self):
        """下降needle命令"""
        return self.generate_command(6, "")

class Node3(NodeCommandGenerator):
    def __init__(self, node_id):
        super().__init__(node_id)
    def configure_liquid(self, liquid_config):
        """配置液体命令"""
        return self.generate_command(2, liquid_config)

    def set_pump_speed(self, speed_config):
        """设置泵速度命令"""
        return self.generate_command(3, speed_config)

    def query_pump_speed(self):
        """查询泵速度命令"""
        return self.generate_command(4, "")

    def manual_control_x_motor(self):
        """手动控制 X motor 命令"""
        return self.generate_command(5, "")

    def manual_control_y_motor(self):
        """手动控制 Y motor 命令"""
        return self.generate_command(6, "")

    def manual_control_z_motor(self):
        """手动控制 Z motor 命令"""
        return self.generate_command(7, "")

class CommandParser:
    @staticmethod
    def parse_node_id(command):
        match = re.findall(r'\$([0-9]+)', command)
        if match:
            return int(match[0])
        else:
            return None

    @staticmethod
    def parse_xyz(command):
        matches = re.findall(r'[xyz]([0-9]+)', command)
        total_step = sum(int(element) for element in matches)
        if matches:
            return total_step
        else:
            return None

    @staticmethod
    def parse_x(command):
        match = re.findall(r'x([0-9]+)', command)
        if match:
            return int(match[0])
        else:
            return None

    @staticmethod
    def parse_y(command):
        match = re.findall(r'y([0-9]+)', command)
        if match:
            return int(match[0])
        else:
            return None

    @staticmethod
    def parse_z(command):
        match = re.findall(r'z([0-9]+)', command)
        if match:
            return int(match[0])
        else:
            return None

class EmailSender:
    def __init__(self, sender_email, sender_password, smtp_server, port):
        self.sender_email = sender_email
        self.sender_password = sender_password
        self.smtp_server = smtp_server
        self.port = port

    def connect(self):
        with smtplib.SMTP(self.smtp_server, self.port) as server:
            server.starttls()
            server.login(self.sender_email, self.sender_password)
            return server

    def send_email(self, receiver_email, subject, message_text):
        message = MIMEText(message_text)
        message["Subject"] = subject
        message["From"] = self.sender_email
        message["To"] = receiver_email

        with self.connect() as server:
            server.sendmail(self.sender_email, receiver_email, message.as_string())