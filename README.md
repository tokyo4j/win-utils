# Set up windows

1. Install some packages:

```
winget install -e --scope machine --id MartinStorsjo.LLVM-MinGW.UCRT
winget install -e --scope machine --id Microsoft.VisualStudioCode
winget install -e --scope machine --id Git.Git
winget install -e --scope machine --id Mozilla.Firefox.DeveloperEdition
winget install -e --scope machine --id Nushell.Nushell
winget install -e --scope machine --id Python.Python.3.11
winget install -e --scope machine --id Alacritty.Alacritty
```

2. Configure registries and schedule key-remapper

```
./setup.batch
```

2. Copy configuration files

```
python ./compare.py
```
