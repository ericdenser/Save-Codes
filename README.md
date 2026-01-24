# Installing ESP-IDF Extension on VSCode

> This guide explains how to install and use **ESP-IDF v5.5.2** on a Debian-based Linux workstation.
> For this tutorial, you wont need permissions to run commands as sudo.

---

## 📌 Prerequisites

- **VSCode**: Ensure you have Visual Studio Code installed on your machine. (If not, check the [tutorial]())

---

## 1. Installing Extension in VSCode

1. Open VSCode and go to the Extensions section (`Ctrl + Shift + X`).
2. Install the following extension: ESP-IDF
- ⚠️ Note: After installing, a setup window might appear. Close it or ignore it for now. Do not continue with the automatic configuration yet.

## 2. Installing Miniconda

ESP-IDF requires a compatible Python version. We use **Miniconda** to manage everything locally.

```bash
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
~/miniconda3/bin/conda init bash
```

- Open a **new terminal** after this step.

---

## 3. Creating the Conda Environment (Safe Setup)

Required build tools are installed **inside Conda**.

```bash
conda create -n esp-idf python=3.11 pip cmake ninja git -y
conda activate esp-idf
```

This environment will be used by the ESP-IDF.

---

## 4. Downloading ESP-IDF

- Note: The command below installs the version that was the most recent at the time this tutorial was written. To ensure you have the newest version, check the [Espressif repository]() and update the version tag if needed.
```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.5.2
cd esp-idf-v5.5.2
```

---

## 5. Installing ESP-IDF Tools

```bash
./install.sh
```

This step installs all required toolchains and creates the internal ESP-IDF Python environment under `.espressif/`.

---

## 6. Configuring the VSCode Extension

Now that everything is installed manually, we need to tell VS Code where to look.

1. Open VS Code.

2. Press F1 (or Ctrl+Shift+P) to open the Command Palette.

3. Type and select: `ESP-IDF: Configure ESP-IDF extension`.

4. Select the option: `USE EXISTING SETUP`.

5. Click "Select ESP-IDF in system."

You will see a window like this: 


### Fill the paths as follows:

- ESP-IDF Path: Select the folder where you downloaded esp-idf (if youre following the same paths, it should be here ~/esp/esp-idf-v5.5.2).

- Python Path: Select the Python interpreter from your Conda environment. (It usually appears as .../miniconda3/envs/esp-idf/bin/python).

---

## ✅ Verification

After the install is complete, a home page window will appear, select `Create project`.
If the page isnt opening, go to the Command Palette again and search for `ESP-IDF: New Project`.

Both ways will lead you to this page:





Now open Command Pallete and search for "ESP-IDF: Open ESP-IDF Terminal".
A new terminal should appear, type the command below on it.

```bash
idf.py --version
```

If the command is recognized, the setup is complete. If not, check the [Common Issue](CommonIssue) section.

## Testing the Project

You can now run this command for building the example project, flashing it into your device and monitoring the serial communication. Dont forget to have your esp plugged.
```bash
idf.py build flash monitor
```

---

## Common Issue

When VS Code is opened from outside our conda environment, the integrated terminal **does not inherit** the ESP-IDF settings.

If you see:

```text
idf.py: command not found
```

Run the following **inside the VS Code terminal**:

```bash
source ~/esp/esp-idf-v5.5.2/export.sh
```
- Change the path of the `export.sh` file if needed.

---

## 5. Notes

* No system packages or sudo permissions are required
* Do **not** rely on internal `.espressif/python_env/...` paths in documentation

---

## References
