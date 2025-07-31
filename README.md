How to build?

This project uses CMake. The build steps vary slightly depending on your operating system.

### Windows

To build on Windows with Visual Studio (e.g., Visual Studio 2022):

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" # Or your installed Visual Studio version
cmake --build .
```

### Linux/macOS

To build on Linux or macOS:

```bash
mkdir -p build
cd build
cmake ..
make
```

How to run?

### Windows

After building, navigate to the output directory (e.g., `build\Debug` or `build\Release`) and run the executables:

In one terminal (for the server):

```powershell
cd build\Debug # Or build\Release
.\chat_server.exe
```

In another terminal (for the client):

```powershell
cd build\Debug # Or build\Release
.\chat_client.exe
```

### Linux/macOS

After building, navigate to the `build` directory and run the executables:

In one terminal (for the server):

```bash
cd build
./chat_server
```

In another terminal (for the client):

```bash
cd build
./chat_client
```
