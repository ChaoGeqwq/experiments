# Google Password Checkup 协议实现

## 项目描述

本项目实现了基于论文 "Private Set Intersection for Password Checkup" 的Google Password Checkup协议。该协议允许用户私密地检查他们的密码是否在已知的泄露密码数据库中，而不向服务器泄露具体的密码内容。

## 协议概述

协议基于隐私集合交集(Private Set Intersection, PSI)技术：

1. **服务器端(Google)**：维护一个已泄露密码的哈希集合
2. **客户端(用户)**：想要检查自己的密码是否在泄露数据库中
3. **协议流程**：使用PSI协议确保用户密码的隐私性

## 文件结构

- `server.py`: 服务器端实现（维护泄露密码数据库）
- `client.py`: 客户端实现（用户密码检查）
- `crypto_utils.py`: 加密工具函数
- `protocol.py`: 核心PSI协议实现
- `demo.py`: 演示脚本
- `requirements.txt`: 依赖包列表

## 使用方法

1. 安装依赖：`pip install -r requirements.txt`
2. 运行演示：`python demo.py`

## 安全特性

- 用户密码不会以明文形式发送给服务器
- 服务器无法了解用户的具体密码
- 用户只能了解自己的密码是否泄露，无法获取泄露数据库的其他信息
