# Installing ESP-IDF v5.5.2 on Debian Workstation (No sudo)

> This guide explains how to install and use **ESP-IDF v5.5.2** on a Debian-based Linux workstation **without sudo permissions**.
> It is written to be **shell-consistent (Bash)** and **self-contained**, avoiding hidden system dependencies.

---

## 📌 Prerequisites

* Debian-based Linux (Bash as default shell)
* No `sudo` access
* Internet access
* Basic tools available (`bash`, `wget`)

---

## 🐍 Installing Miniconda (User-level Python)

ESP-IDF requires a compatible Python version. Since system Python cannot be modified, **Miniconda** is used to manage everything locally.

```bash
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
~/miniconda3/bin/conda init bash
```

🔁 Open a **new terminal** after this step.

---

## 🧪 Creating the Conda Environment (Safe Setup)

To avoid relying on system packages, required build tools are installed **inside Conda**.

```bash
conda create -n esp-idf python=3.11 pip cmake ninja git -y
conda activate esp-idf
python --version
```

This environment will be used by the ESP-IDF installer.

---

## 📥 Downloading ESP-IDF

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.5.2
cd esp-idf-v5.5.2
```

---

## ⚙️ Installing ESP-IDF Tools

```bash
./install.sh
```

This step installs all required toolchains and creates the internal ESP-IDF Python environment under `.espressif/`.

---

## 🌱 Loading the ESP-IDF Environment (IMPORTANT)

ESP-IDF **does not modify** `.bashrc` automatically.
The environment must be loaded **every time a new terminal is opened**.

```bash
source ~/esp/esp-idf-v5.5.2/export.sh
```

After this command, `idf.py` becomes available in the current shell.

---

## ✅ Verification

```bash
idf.py --version
```

If the command is recognized, the setup is complete 🎉

---

## 🧑‍💻 Using ESP-IDF with VS Code (Common Issue)

When VS Code is opened from the graphical interface, the integrated terminal **does not inherit** the ESP-IDF environment.

If you see:

```text
idf.py: command not found
```

Run the following **inside the VS Code terminal**:

```bash
source ~/esp/esp-idf-v5.5.2/export.sh
```

---

## ✨ Optional: Convenience Alias (Bash)

To avoid typing the full command every time, you may create an alias **for Bash users**:

```bash
echo "alias get_idf='source ~/esp/esp-idf-v5.5.2/export.sh'" >> ~/.bashrc
source ~/.bashrc
```

Usage:

```bash
get_idf
```

---

## 📝 Notes

* This guide assumes **Bash** (default on Debian)
* No system packages or sudo permissions are required
* Do **not** rely on internal `.espressif/python_env/...` paths in documentation
* Multiple ESP-IDF versions can coexist using this method

---

## 📚 References

