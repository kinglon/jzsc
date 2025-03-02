import sys
import os
import datetime
import threading


class PrintStream:
    lock = threading.Lock()

    def __init__(self, streams):
        self.streams = streams

    def write(self, data):
        if data == '\n':
            return

        with PrintStream.lock:
            for stream in self.streams:
                current_time = datetime.datetime.now()
                formatted_time = current_time.strftime("[%Y-%m-%d %H:%M:%S]")
                thread_id = '[{}]'.format(threading.current_thread().ident)
                stream.write(formatted_time + thread_id + ' ' + data + '\n')
                stream.flush()

    def flush(self):
        for stream in self.streams:
            stream.flush()


class LogUtil:
    file_name_prefix = 'main'

    @staticmethod
    def enable():
        current_file_path = os.path.dirname(os.path.abspath(__file__))
        log_path = os.path.join(current_file_path, 'logs')
        os.makedirs(log_path, exist_ok=True)

        current_date = datetime.datetime.now()
        log_file_name = LogUtil.file_name_prefix + current_date.strftime("_%Y%m%d_%H%M.log")
        log_file_path = os.path.join(log_path, log_file_name)
        log_stream = open(log_file_path, 'a', encoding='utf-8')
        sys.stdout = PrintStream([sys.stdout, log_stream])
        sys.stderr = PrintStream([sys.stderr, log_stream])
