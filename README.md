# Installing ESP-IDF Extension on VSCode

> This guide explains how to install and use **ESP-IDF v5.5.2** on a Debian-based Linux workstation.
> For this tutorial, you wont need permissions to run comands as sudo.

---

## 📌 Prerequisites

- **VSCode**: Ensure you have Visual Studio Code installed on your machine.

---

## 1. Installing Extension in VSCode

1. Open VSCode and go to the Extensions section (`Ctrl + Shift + X`).
2. Install the following extension: ESP-IDF
- ⚠️ Attention: After installing, a setup window might appear. Close it or ignore it for now. Do not continue with the automatic configuration yet.

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

- Note: The command below installs the version that was the most recent at the time this tutorial was written. To ensure you have the newest version, check the Espressif repository and update the version tag if needed.
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

3. Type and select: ESP-IDF: Configure ESP-IDF extension.

4. Select the option: USE EXISTING SETUP.

### Fulfill the paths as follows:

- ESP-IDF Path: Select the folder where you downloaded esp-idf (if your following the same paths, it should be here ~/esp/esp-idf-v5.5.2).

- Python Path: Select the Python interpreter from your Conda environment. (It usually appears as .../miniconda3/envs/esp-idf/bin/python).

---

## ✅ Verification

```bash
idf.py --version
```

If the command is recognized, the setup is complete.

---

## Common Issue

When VS Code is opened from outside our conda enviroment, the integrated terminal **does not inherit** the ESP-IDF settings.

If you see:

```text
idf.py: command not found
```

Run the following **inside the VS Code terminal**:

```bash
source ~/esp/esp-idf-v5.5.2/export.sh
```

---

## 5. Notes

* No system packages or sudo permissions are required
* Do **not** rely on internal `.espressif/python_env/...` paths in documentation

---

## References
