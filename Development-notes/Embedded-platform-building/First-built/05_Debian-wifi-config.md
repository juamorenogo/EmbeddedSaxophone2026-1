
# Dependencies Installation Using Docker (Debian Bookworm, ARM Target)

## Overview

This procedure establishes a **controlled and reproducible method** to install WiFi-related dependencies for an embedded Debian system (ARMHF), without breaking the target system due to dependency mismatches.

The key idea is:

> Use a **clean Debian Bookworm environment inside Docker** to download all required `.deb` packages, then transfer and install them manually on the embedded system.

This avoids issues such as:

- Broken dependencies
- Incompatible libc versions
- Mixing Debian releases (e.g., Bookworm vs Trixie)

## Why Docker is Used

The embedded system:

- Has limited or no internet access
- Cannot reliably resolve dependencies via `apt`
- May break easily if incorrect packages are installed

Docker provides:

- A clean Debian Bookworm userspace
- Full access to official repositories
- A safe environment to resolve dependencies

## Architecture Consideration (armhf)

The target system uses **ARM (armhf)** architecture.

However, Docker runs on an **amd64 host**, so we must explicitly enable multi-architecture support:

```bash
dpkg --add-architecture armhf
```

This allows downloading packages for the ARM target while running on x86.

---

### Step 1 — Start Docker Container

A Debian Bookworm container is launched:

```bash
docker run -it debian:bookworm bash
```

This ensures:

- Matching distribution (Bookworm)
- Clean package database
- No contamination from host system

### Step 2 — Update Package Index

```bash
apt update
```

Required to fetch the latest package metadata before downloading anything.

### Step 3 — Enable ARM Architecture

```bash
dpkg --add-architecture armhf
apt update
```

Why:
- Without this, `apt` only knows about amd64 packages
- We need ARM packages for the embedded board

### Step 4 — Download Required Packages

Instead of installing, we **download `.deb` files only**:

```bash
apt download iw:armhf wpasupplicant:armhf wireless-tools:armhf wireless-regdb:all
```

Then manually resolve missing dependencies:

```bash
apt download \
    libnl-3-200:armhf \
    libnl-genl-3-200:armhf \
    libnl-route-3-200:armhf \
    libdbus-1-3:armhf \
    libiw30:armhf \
    libpcsclite1:armhf
```

### Why this step is critical

Direct installation on the embedded system failed because:

- Dependencies were missing
- `apt` could not resolve them automatically
- Manual `.deb` installation requires all dependencies present

This step ensures:

- All required libraries are collected
- Correct versions (Bookworm-compatible)

### Step 5 — Organize Packages

Create a safe directory inside the container:

```bash
mkdir /safe-debs
mv *.deb /safe-debs/
```

Purpose:

- Avoid mixing with unrelated packages
- Ensure only required dependencies are transferred

### Step 6 — Copy Packages to Host

From the host system:

```bash
docker cp <container_id>:/safe-debs ./debs
```

This extracts all downloaded `.deb` files from Docker.

### Step 7 — Transfer to SD Card

Mount the embedded system root filesystem:

```bash
mount /dev/sda2 /mnt/sd
```

Copy packages:

```bash
sudo cp ./debs /mnt/sd/root/
```

Why `/root/`:

- Guaranteed write permissions
- Accessible during early debugging

### Step 8 — Install on Target System

On the embedded board:

```bash
cd /root/debs
dpkg -i *.deb
```

Why `dpkg`:

- No dependency resolution required (already handled)
- Works offline


### Step 9 — Verify Installation

Test tools:

```bash
iw dev
ip link
```

If successful:

- WiFi interface appears
- Driver is functional
- Userspace tools are operational

## Common Pitfalls

### 1. Mixing Debian Versions

Installing packages from **Trixie** on a **Bookworm system** leads to:

- libc mismatch
- Kernel panic (`Attempted to kill init`)

### 2. Missing Dependencies

Symptoms:

- `dependency problems - leaving unconfigured`

Solution:

- Always download dependencies from Docker first
