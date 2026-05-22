@echo off
cd /d %~dp0
start cmd /k "pip install -i https://pypi.tuna.tsinghua.edu.cn/simple pandas openpyxl numpy==1.26.0 comtypes requests pywin32 playsound cryptography"