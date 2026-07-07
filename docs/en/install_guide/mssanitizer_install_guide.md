# MindStudio Sanitizer Installation Guide

<br>

## 1. Installation Notes

This tool is integrated into CANN. If CANN is already installed and you do not need to update this tool, you can use it directly without following this document for installation.

If CANN is not yet installed in your environment, see [CANN Quick Install](https://www.hiascend.com/cann/download) to install the Ascend NPU driver and CANN software (including the Toolkit and ops packages), and configure the environment variables.

To upgrade this tool separately or use the latest version, you can install it in one of the following ways: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), [Source Code Installation](#23-source-code-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has internet access, you can automatically download and install the tool with a single command. Go to the MindStudio [Download](https://www.hiascend.com/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, and choose "Online Installation" as the installation method. The system then guides you through the remaining steps.

### 2.2 Offline Installation

For devices in an enterprise intranet or other environments without internet access, first download the complete offline installation package on a machine with internet access, and then transfer it to the target device for installation. Go to the MindStudio [Download](https://www.hiascend.com/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, and choose "Offline Installation" to obtain the installation package and instructions.

### 2.3 Source Code Installation

#### 2.3.1 Environment Setup

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/26.0.0/docs/en/common/dev_env_setup.md).

#### 2.3.2 Building and Packaging

- Clone this repository.

    ```sh
    git clone https://gitcode.com/Ascend/mssanitizer.git -b 26.0.0
    ```

- Run the one-click script to automatically download and build the dependency repository:

    ```shell
    cd mssanitizer
    python build.py
    ```

#### 2.3.3 Installation

##### 2.3.3.1 Preparing the .run Package

The .run package is generated in the `output` directory. Run the following commands to ensure that the .run package has the execute permission:

```shell
cd output
chmod +x mindstudio-sanitizer_*.run
```

##### 2.3.3.2 Installation

Copy the .run package to the operating environment (not required for local installation) and perform the following operation:

```shell
./mindstudio-sanitizer_*.run --run
```

If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter "y" to perform an overwrite installation.

> [!NOTE]
>
> Installation path description.
>
> If the `ASCEND_HOME_PATH` environment variable has been configured in the environment, the tool will be installed in the `$ASCEND_HOME_PATH` directory.
> Otherwise, the tool will be installed in the `$HOME/Ascend` directory by default.
> To specify a custom installation path, use the `--install-path` option. For example:
> `./mindstudio-sanitizer_*.run --install-path=./xxx --run` installs the runfile to the `xxx` directory.

## 3. Verifying the Installation

After the installation is complete, run the following command to verify that the tool is successfully installed:

```shell
mssanitizer --help
```

If no error is reported and the help information is displayed, the installation is successful.

## 4. Uninstallation

You can uninstall by following these steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.0.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - A network connection is required to download the script. If the environment does not allow internet access or is offline, download the script on a machine with internet access first and copy it to the target device.
   > - If the command does not respond or encounters a connection failure, SSL certificate error, or similar issue, see [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Perform the uninstallation.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can query the tool name by running `python ms_install.py help`; the tool name is displayed under the Available Tools field in the output.

   If the uninstallation is successful, the following information is printed:

   ```ColdFusion
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

Simply run the installation command. The tool automatically uninstalls the old version and guides you through the overwrite installation.

## 6. FAQ

### The Newly Compiled Tool Is Not Being Invoked After Installation

Run the following commands to check whether the related environment variables are correctly configured to ensure that the system preferentially uses the newly installed operator tool:

```shell
export ASCEND_HOME_PATH=$HOME/Ascend  # Or export ASCEND_HOME_PATH=$PWD/xxx (for custom installation paths)
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

### How Do I Uninstall the Software When the .run Package Has Been Deleted?

You can run the following command to perform the uninstallation:

```shell
bash $HOME/Ascend/cann/share/info/mindstudio-sanitizer/script/uninstall.sh
```

For a custom installation path, use the uninstallation script in the corresponding path:

```shell
bash ./xxx/share/info/mindstudio-sanitizer/script/uninstall.sh
```
