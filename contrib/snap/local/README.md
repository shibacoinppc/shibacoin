# Shibacoin Snap Packaging

Commands for building and uploading a Shibacoin Core Snap to the Snap Store. Anyone on amd64 (x86_64), arm64 (aarch64), or i386 (i686) should be able to build it themselves with these instructions. This would pull the official Shibacoin binaries from the releases page, verify them, and install them on a user's machine.

## Building Locally
```
sudo apt install snapd
sudo snap install --classic snapcraft
sudo snapcraft
```

### Installing Locally
```
snap install \*.snap --devmode
```

### To Upload to the Snap Store
```
snapcraft login
snapcraft register shibacoin-core
snapcraft upload \*.snap
sudo snap install shibacoin-core
```

### Usage
```
shibacoin-unofficial.cli # for shibacoin-cli
shibacoin-unofficial.d # for shibacoind
shibacoin-unofficial.qt # for shibacoin-qt
shibacoin-unofficial.test # for test_shibacoin
shibacoin-unofficial.tx # for shibacoin-tx
```

### Uninstalling
```
sudo snap remove shibacoin-unofficial
```