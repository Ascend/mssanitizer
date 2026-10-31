# msSanitizer Installation Guide

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

To use the functions of the latest code or modify the source code to enhance functions, you can download the code from this repository, build and package the tool, and install it.

#### 2.3.1 Environment Setup

All source code compilation uses the standard MindStudio build environment.

- For daily development or when you use a released image, refer to the [MindStudio Tool Development Environment Installation Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/dev_env_setup.md).
- If you need to reproduce the environment from a base operating system, or to perform source code build verification or unit test verification, you must refer to the [MindStudio Unified Build Image Creation Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/docker_image_build_guide.md) to build the environment image on site from the openEuler base image.

The source code compilation and unit test commands later in this document are all run in the container of the specified image or of the environment image built on site. The CANN software package version, GCC version, and Python version are subject to the Unified Build Image Creation Guide, and this repository does not maintain them separately.

After the image is built, you must use the `ctr_in.py` command provided in Chapter 7 of the Unified Build Image Creation Guide to start and enter the container in an interactive terminal. Do not use a plain `docker run` to create a container, and do not use `docker exec <container_name> bash -c '<command>'` to replace the interactive environment. Otherwise, the initialization of the Python, GCC, and CANN environments may be skipped.

After you enter the interactive container shell opened by `ctr_in.py`, run the following command to clone this repository:

```bash
cd ~
git clone https://gitcode.com/Ascend/mssanitizer.git
```

#### 2.3.2 Compilation

Stay in the same interactive container shell opened by `ctr_in.py`, and run the following command in the repository root directory to automatically download dependencies and build:

```bash
cd ~/mssanitizer
python3 build.py
```

After the build succeeds, the installation package is generated in the `artifacts/` directory.

#### 2.3.3 Unit Tests (Optional)

This step is not required for installation. If you need to verify the basic functions of the code, you can run the unit tests:

```bash
cd ~/mssanitizer
python3 build.py test
```

If the return code of the command is 0 and no test case fails, the unit tests pass.

#### 2.3.4 Installation

##### 2.3.4.1 Preparing the .run Package

Run the following command to ensure that the .run package has the execute permission:

```bash
chmod +x mindstudio-sanitizer_*.run
```

##### 2.3.4.2 Installation

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

If you are prompted that the command is not found, see [FAQ](#6-faq) to check the environment variable configuration.

## 4. Uninstallation

You can uninstall by following these steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.2.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - A network connection is required to download the script. If the environment does not allow internet access or is offline, download the script on a machine with internet access first and copy it to the target device.
   > - If the command does not respond or encounters a connection failure, SSL certificate error, or similar issue, see [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Perform the uninstallation.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can query the tool name by running `python ms_install.py help`. The tool name is displayed under the Available Tools field in the output.

   If the uninstallation is successful, the following information is printed:

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

Simply run the installation command. The tool automatically uninstalls the old version and guides you through the overwrite installation. You can also uninstall the tool manually and then install it again:

- Uninstall the old version first (see Section 4).
- Then install the new version by following the methods in Section 2.

You can run the `mssanitizer --version` command to view the version information about the current environment and then select the version to upgrade. When upgrading the version, pay attention to the version matching relationships. For details, see [Release Notes](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/master/release_notes.md).

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
