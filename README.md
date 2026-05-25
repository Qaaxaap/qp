# qp – A Clean, C++ Wrapper for the qur Repository

> *Convenient package management – without hiding how your system is built*

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

**qp** is a lightweight, C++ based user tool for the [qur](https://github.com/qaaxaap/qur) package repository. It is designed to be simple, fast, and completely transparent.

- You never need to manually clone `qur` – `qp` fetches metadata automatically.
- Every package is built from source using a readable `MAKEFS` script.
- You can always bypass `qp` and run `./MAKEFS` by hand.
- Under the hood, `lfspkg` handles package tracking and ELF dependency parsing – but you don’t need to worry about those details. `qp` gives you a clean interface while keeping everything visible.

**Repository:** [https://github.com/qaaxaap/qp.git](https://github.com/qaaxaap/qp.git)

---

## Philosophy

> *“Automate the repetitive tasks, but never hide the build process.”*

`qp` does not invent a new package format. It does not obfuscate build steps. It simply:

- Resolves dependencies for you
- Downloads the correct `MAKEFS` scripts from the `qur` repository
- Runs them in a controlled environment
- Keeps a log of installed files (so you can uninstall cleanly)

If you ever want to see exactly what `qp` would do – just read the `MAKEFS` script. It is right there in `packages/<name>/`.

---

## A Unique Feature: Manual Registration

Unlike most package managers, `qp` does **not** assume that it is the only way to install software on your system. You are free to compile from source by hand, use alternative build systems, or even install pre‑compiled binaries. `qp` helps you stay organised by allowing you to **manually register** such external installations into its local database.

- **`qp register <pkg> [version] [dep1,dep2,...]`** – tells `qp` that a package is already installed (e.g., you ran `./MAKEFS` manually or used `make install`). You can optionally specify its version and dependencies.
- **`qp unregister <pkg>`** – removes a package from the database without touching the actual files (useful when you manually uninstalled it).

This feature ensures that `qp list`, dependency resolution, and removal commands remain consistent with the real state of your system – even if you sometimes work outside the tool. It puts you back in control, not the package manager.

### Working Alongside Other Package Managers (e.g., apt)

Because you can manually register dependencies, `qp` does **not** require a “pure” LFS system. You can run `qp` on a machine that already uses `apt`, `dnf`, or any other package manager. Simply register the libraries and tools that are already installed by those managers, and `qp` will resolve them correctly when building packages from `qur`. The resulting software will work side‑by‑side with your existing packages – no conflicts, no forced migration. This makes `qp` a lightweight, non‑invasive addition to any Linux distribution, not just your LFS‑based one.

In short: **`qp` respects your existing system and coexists with it. You stay in control.**

---

## Quick Start

### 1. Clone and build `qp`

```bash
git clone https://github.com/qaaxaap/qp.git
cd qp
make
sudo make install
```

### 2. Use `qp`

After installation, `qp` is available globally:

```bash
qp --help
```

### 3. Search and install a package

```bash
qp search vim
qp install vim
```

`qp` will pull the latest metadata from `qur`, resolve dependencies, and build `vim` from source using its `MAKEFS` script.

### 4. List installed packages

```bash
qp list
```

### 5. Remove a package

```bash
qp remove vim
```

### 6. Rebuild a package (e.g., after editing its `MAKEFS`)

```bash
qp rebuild vim
```

### 7. Update local repository metadata

```bash
qp update
```

### 8. Register / unregister a manually installed package

```bash
qp register <pkg> [version] [dep1,dep2,...]
qp unregister <pkg>
```

Use these commands to keep the database in sync when you install or remove software by hand (outside of `qp`). This is a key feature that respects your freedom to manage packages outside the tool.

---

## How `qp` Works

When you run `qp install <pkg>`:

1. `qp` reads the local repository cache (mirroring `qur`’s `meta/` directory).
2. It uses `lfspkg` (the underlying tracker) for dependency resolution and ELF parsing – but you never need to invoke `lfspkg` directly.
3. For each package, `qp` locates the `MAKEFS` script, sets environment variables (like `$DESTDIR`), and executes the `build()` and `install()` functions defined in the script.
4. It records every installed file path in a log.

Because `MAKEFS` scripts are standard bash, you can reproduce every step by hand. No magic, no binary blobs.

---

## Configuration

`qp` reads `/etc/qp.conf` (or `~/.config/qp/qp.conf`). Example:

```
# qp configuration
REPO_URL = "https://github.com/qaaxaap/qur.git"
REPO_DIR = "/var/lib/qur"
BUILD_DIR = "/tmp/qp-build"
LOG_DIR = "/var/log/qp"
```

If the configuration file is missing, sensible defaults are used.

---

## FAQ

**Q: Do I still need to understand `MAKEFS` scripts?**  
A: No, but you are encouraged to. `qp` works fine if you just want to install software. However, the whole point of `qur` is that you *can* understand and modify them – `qp` does not stand in your way.

**Q: How is `qp` different from `lfspkg`?**  
A: `lfspkg` is the low‑level component that tracks installed packages and parses ELF dependencies. `qp` is the user‑friendly wrapper that you type every day. You can use either; `qp` simply hides the details you don’t need to see.

**Q: Why does `qp` have manual registration? Most package managers don't need that.**  
A: Because most package managers assume they are the sole source of truth. `qp` respects that you might want to install software manually – for example, when you are experimenting with a custom build, or when a package is not yet in `qur`. Manual registration lets you keep a single, consistent view of *everything* installed on your system, whether `qp` did it or you did it yourself.

**Q: Can I use `qp` without an internet connection?**  
A: Yes, as long as you have previously run `qp update` or cloned the `qur` repository locally. `qp` will work offline using the cached metadata and source tarballs (if they are already downloaded).

**Q: Why not just use `pacman`/`apt`/`dnf`?**  
A: Those are excellent tools for their ecosystems, but they are black boxes. `qp` + `qur` gives you complete visibility and control – every build script is a plain text file you can edit, run, or branch.

**Q: What does “qp” stand for?**  
A: It can stand for “Quick Package” – or anything you like. After all, this is **your** package manager.

---

## Contributing

`qp` is developed on GitHub: [https://github.com/qaaxaap/qp](https://github.com/qaaxaap/qp). To contribute:

1. Fork the repository (create your own copy on GitHub).
2. Clone your fork locally.
3. Create a branch for your change.
4. Make your changes (C++ code, following the existing style).
5. Submit a pull request.

Code style: keep it clean, commented, and consistent with the surrounding code.

---

## License

`qp` is released under the **GNU General Public License v3.0 or later**, the same as the `qur` repository.

---

**Enjoy the freedom – your system, your wrapper, your rules.**

---

# qp – 一个简洁的、C++ 编写的 qur 仓库封装工具

> *便利的包管理 – 但不隐藏系统的构建过程*

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

**qp** 是一个基于 C++ 的轻量级用户工具，用于 [qur](https://github.com/qaaxaap/qur) 软件包仓库。它被设计得简单、快速且完全透明。

- 你永远不需要手动克隆 `qur` – `qp` 会自动获取元数据。
- 每个软件包依然通过可读的 `MAKEFS` 脚本从源码构建。
- 你可以随时绕过 `qp`，手动执行 `./MAKEFS`。
- 在底层，`lfspkg` 负责包跟踪和 ELF 依赖解析 – 但你无需关心这些细节。`qp` 为你提供一个干净的界面，同时保持一切可见。

**仓库地址：** [https://github.com/qaaxaap/qp.git](https://github.com/qaaxaap/qp.git)

---

## 理念

> *“自动化重复的任务，但绝不隐藏构建过程。”*

`qp` 没有发明新的软件包格式，也不混淆构建步骤。它仅仅：

- 为你解析依赖关系
- 从 `qur` 仓库下载正确的 `MAKEFS` 脚本
- 在受控环境中运行它们
- 记录安装的文件清单（以便你能干净地卸载）

如果你任何时候想确切知道 `qp` 会做什么 – 直接阅读 `MAKEFS` 脚本即可。它就放在 `packages/<name>/` 里。

---

## 独特特性：手动登记

与大多数包管理器不同，`qp` **从不假设**它是你系统上安装软件的唯一途径。你可以自由地手动编译源码、使用其他构建系统，甚至直接安装预编译的二进制包。`qp` 通过允许你**手动登记**这类外部安装来帮助你保持系统记录的一致性。

- **`qp register <pkg> [version] [dep1,dep2,...]`** – 告知 `qp` 某个软件包已经安装好了（例如你手动运行了 `./MAKEFS` 或 `make install`）。你可以选择指定版本和依赖列表。
- **`qp unregister <pkg>`** – 将软件包从数据库中移除，但不删除实际文件（当你手动卸载了它时很有用）。

这一特性确保 `qp list`、依赖解析和卸载命令始终与系统的真实状态保持一致 – 即使你有时会脱离工具直接操作。它把控制权交还给你，而不是包管理器。

### 与其他包管理器（例如 apt）共存

因为你可以手动登记依赖，`qp` **并不要求**一个“纯净”的 LFS 系统。你甚至可以在已经安装了 `apt`、`dnf` 或其他包管理器的机器上运行 `qp`。只需将那些已由其他包管理器安装好的库和工具手动登记一下，`qp` 在从 `qur` 构建软件包时就能正确解析它们。最终构建出来的软件可以和现有软件包和平共处——没有冲突，也无需强制迁移。这使得 `qp` 成为一个轻量、无侵入的附加工具，不仅适用于你基于 LFS 的发行版，也适用于任何 Linux 发行版。

简而言之：**`qp` 尊重你的现有系统，与它共存。控制权始终在你手中。**

---

## 快速开始

### 1. 克隆并构建 `qp`

```bash
git clone https://github.com/qaaxaap/qp.git
cd qp
make
sudo make install
```

### 2. 使用 `qp`

安装完成后，`qp` 即可全局使用：

```bash
qp --help
```

### 3. 搜索并安装软件包

```bash
qp search vim
qp install vim
```

`qp` 会拉取 `qur` 的最新元数据，解析依赖，然后使用 `vim` 的 `MAKEFS` 脚本从源码构建它。

### 4. 列出已安装的软件包

```bash
qp list
```

### 5. 卸载软件包

```bash
qp remove vim
```

### 6. 重新构建软件包（比如修改了它的 `MAKEFS` 之后）

```bash
qp rebuild vim
```

### 7. 更新本地仓库元数据

```bash
qp update
```

### 8. 登记/取消登记某软件包已经被手动安装

```bash
qp register <pkg> [version] [dep1,dep2,...]
qp unregister <pkg>
```

当你不通过 `qp` 手动安装或卸载软件时，使用这些命令保持数据库同步。这是一个尊重你自由的关键特性 —— 你可以在工具之外自行管理软件。

---

## `qp` 的工作原理

当你执行 `qp install <pkg>` 时：

1. `qp` 读取本地仓库缓存。
2. 它使用 `lfspkg`（底层跟踪器）进行依赖解析和 ELF 解析 – 但你永远不需要直接调用 `lfspkg`。
3. 对于每个软件包，`qp` 找到对应的 `MAKEFS` 脚本，设置环境变量（如 `$DESTDIR`），然后执行脚本中定义的 `build()` 和 `install()` 函数。
4. 将每个安装的文件路径记录到日志中。

因为 `MAKEFS` 脚本是标准 bash 脚本，你可以亲手重现每一步。没有魔法，没有二进制大包。

---

## 配置

`qp` 会读取 `/etc/qp.conf`（或 `~/.config/qp/qp.conf`）。示例：

```
# qp 配置文件
REPO_URL = "https://github.com/qaaxaap/qur.git"
REPO_DIR = "/var/lib/qur"
BUILD_DIR = "/tmp/qp-build"
LOG_DIR = "/var/log/qp"
```

如果配置文件不存在，将使用合理的默认值。

---

## 常见问题

**问：我仍然需要理解 `MAKEFS` 脚本吗？**  
答：不需要，但我们鼓励你去理解。如果你只是想安装软件，`qp` 也能正常工作。然而，`qur` 的全部意义在于你**可以**理解和修改这些脚本 – `qp` 不会成为你的障碍。

**问：`qp` 和 `lfspkg` 有什么区别？**  
答：`lfspkg` 是底层的组件，负责跟踪已安装的包并解析 ELF 依赖。`qp` 是每天供你使用的、对用户友好的封装工具。你可以任选其一；`qp` 只是隐藏了你不需要看到的细节。

**问：为什么 `qp` 需要手动登记？大多数包管理器都不需要这个。**  
答：因为大多数包管理器假设它们是唯一的真相来源。`qp` 尊重你手动安装软件的可能性 – 例如当你在实验自定义构建时，或者某个软件包尚未被 `qur` 收录时。手动登记让你能够在一个统一的视图里看到**所有**安装在系统上的软件，无论它们是由 `qp` 安装的还是你自己动手安装的。

**问：我可以在没有网络连接的情况下使用 `qp` 吗？**  
答：可以，只要你之前运行过 `qp update` 或者已经本地克隆了 `qur` 仓库。`qp` 会离线使用缓存的元数据和源码包（如果已经下载过）。

**问：为什么不直接使用 `pacman`/`apt`/`dnf`？**  
答：那些工具在各自的生态中都非常出色，但它们都是黑盒。`qp` + `qur` 给予你完全的可见性和控制力 – 每个构建脚本都是一个纯文本文件，你可以编辑、运行或分支（branch）。

**问：“qp” 是什么意思？**  
答：它可以代表 “Quick Package” – 或者任何你喜欢的含义。毕竟，这是你的包管理器。

---

## 贡献指南

`qp` 在 GitHub 上开发：[https://github.com/qaaxaap/qp](https://github.com/qaaxaap/qp)。要贡献：

1. Fork 该仓库（在 GitHub 上创建你自己的副本）。
2. 将你的 fork 克隆到本地。
3. 为你的修改创建一个分支（branch）。
4. 进行修改（C++ 代码，遵循现有风格）。
5. 提交 Pull Request。

代码风格：保持整洁、有注释，并与周围代码保持一致。

---

## 许可证

`qp` 采用 **GNU General Public License v3.0 或更高版本**，与 `qur` 仓库一致。

---

**享受自由 – 你的系统，你的封装工具，你的规则。**
