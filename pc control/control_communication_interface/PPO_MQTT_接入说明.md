# PPO 程序需要接入的内容

你的程序只需要接入 `cone_mqtt_controller.py` 提供的 MQTT 发送接口，不需要自己编写 MQTT 底层代码。MQTT 凭据应从环境变量或其他本地配置读取，不要提交到 Git 仓库。

## 1. 导入通信模块

确保 `cone_mqtt_controller.py` 可以被你的运行程序导入：

```python
from cone_mqtt_controller import ConeMqttController
```

## 2. 程序开始时连接一次服务器

```python
controller = ConeMqttController(
    broker_host="MQTT服务器地址",
    broker_port=1883,
    username="MQTT账号",
    password="MQTT密码",
)

controller.start()
```

`controller.start()` 只需要在程序开始时调用一次，不能每发送一条命令就重新连接。

## 3. 把 PPO 输出交给发送接口

PPO 环境已经输出以下五个字段：

```python
info["left_pwm"]
info["right_pwm"]
info["pulses"]
info["angle_left"]
info["angle_right"]
```

得到 `info` 后调用：

```python
controller.send_command(
    cone_id=1,
    left_pwm=info["left_pwm"],
    right_pwm=info["right_pwm"],
    pulses=info["pulses"],
    angle_left=info["angle_left"],
    angle_right=info["angle_right"],
)
```

`cone_id=1` 表示实体锥桶 1，对应 MQTT Topic `cone1d`。不要把仿真环境中的 `cone_index` 直接当作 `cone_id`。

## 4. 程序退出时关闭连接

整个控制过程应放在 `try/finally` 中：

```python
controller.start()

try:
    # PPO 推理和命令发送代码
    ...
finally:
    controller.close(stop_touched=True)
```

`close()` 会尝试停止本次控制过的锥桶，然后断开 MQTT。

## 如果使用修改后的 ConePPOEnv

修改后的环境也可以直接接入发送函数：

```python
env = ConePPOEnv(
    command_sender=controller.send_command,
    physical_cone_id=1,
)
```

这样每次执行：

```python
obs, reward, terminated, truncated, info = env.step(action)
```

环境都会自动发送五字段命令，不需要再手动调用 `controller.send_command()`。

自动发送和手动发送只能选择一种，否则同一条命令会发送两次。
