# MindStudio Sanitizer 安装指南

<br>

## 1. 二进制安装

MindStudio工具链是集成到CANN包中发布的，可通过以下方式完成安装：

### 方式一：依据 CANN 官方文档安装  

请参考<a href="https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst" target="_blank">《CANN安装官方文档》</a>，
按文档逐步安装和配置。

### 方式二：使用CANN官方容器镜像   

请访问<a href="https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884" target="_blank">《CANN官方镜像仓库》</a>，
按照仓库中的指引完成镜像拉取及容器启动。

<br>

## 2. 源码安装

如需使用最新代码的功能，或对源码进行修改以增强功能，可下载本仓库代码，自行编译、打包工具并完成安装。

### 2.1 环境准备

请按照以下文档进行环境配置：[《算子工具开发环境安装指导》](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 2.2 执行编译打包

通过一键式脚本自动完成依赖仓库的下载与构建流程：

```shell
python build.py
```

### 2.3 安装与卸载

#### 2.3.1 准备 run 包

run 包将生成于 output 目录下，执行以下命令确保其具备可执行权限：

```shell
cd output
chmod +x mindstudio-sanitizer_*.run
```

#### 2.3.2 安装

将 run 包拷贝到运行环境中（本机安装无需拷贝），执行如下安装操作：

```shell
./mindstudio-sanitizer_*.run --run
```

若系统中已安装该工具的旧版本，安装过程中会提示是否替换；输入 "y" 可执行覆盖安装。
>[!NOTE]安装路径说明   
> 若环境中已配置 `ASCEND_HOME_PATH` 环境变量，工具将安装至 `$ASCEND_HOME_PATH` 目录；
> 否则，默认安装至 `$HOME/Ascend` 目录；  
> 如需指定自定义安装路径，请使用 `--install-path` 选项，例如：
> `./mindstudio-sanitizer_*.run --install-path=./xxx --run`，即可将该运行包安装至 `xxx` 目录。

#### 2.3.3 卸载

可通过如下命令卸载：

```shell
./mindstudio-sanitizer_*.run --uninstall
```

>[!NOTE]卸载路径说明   
> 默认将在 `$HOME/Ascend` 目录下卸载；若先前安装时通过 `--install-path` 指定了自定义路径，
> 则卸载时也需显式添加 `--install-path` 选项，例如：
> `./mindstudio-sanitizer_*.run --install-path=./xxx --uninstall`。

#### 2.3.4 升级

升级操作本质上是卸载旧版本并安装新版本，与[2.3.2 安装](#232-安装)中所述的覆盖安装方式一致，请参见相关操作说明。

### 2.4 FAQ

#### 2.4.1 安装完成后，执行命令未调用新编译的工具

请参考以下命令，检查相关环境变量是否配置正确，以确保系统优先使用新安装的算子工具：

```shell
export ASCEND_HOME_PATH=$HOME/Ascend  # 或 export ASCEND_HOME_PATH=$PWD/xxx（指定路径安装场景）
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

#### 2.4.2 run 包已删除时如何卸载？

可通过以下命令执行卸载操作：

```shell
bash $HOME/Ascend/share/info/mindstudio-sanitizer/script/uninstall.sh
```

对于指定路径安装的场景，请使用对应路径下的卸载脚本：

```shell
bash ./xxx/share/info/mindstudio-sanitizer/script/uninstall.sh 
```
