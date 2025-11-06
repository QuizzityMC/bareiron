# Building Bareiron for Raspberry Pi Pico W

This guide explains how to build and deploy Bareiron on a Raspberry Pi Pico W microcontroller.

## Prerequisites

### Hardware
- Raspberry Pi Pico W board (~$6 USD)
- USB cable (micro-USB)
- Computer to build and flash

### Software
- Pico SDK
- CMake (version 3.13 or higher)
- ARM GCC toolchain (`arm-none-eabi-gcc`)
- Git

## Setup Instructions

### 1. Install Pico SDK

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential

# Clone Pico SDK
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable
export PICO_SDK_PATH=~/pico-sdk
echo "export PICO_SDK_PATH=~/pico-sdk" >> ~/.bashrc
```

### 2. Configure WiFi Credentials

Edit `include/globals.h`:

```c
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"
```

**Important:** The Pico W only supports 2.4 GHz WiFi networks!

### 3. Generate Registry Data

From the bareiron root directory:

```bash
./extract_registries.sh
# OR follow manual instructions in README.md
```

### 4. Configure Server Settings (Optional)

For best performance on Pico W, adjust these settings in `include/globals.h`:

```c
// Memory-constrained settings for Pico W
#define MAX_PLAYERS 2              // Very limited players
#define MAX_BLOCK_CHANGES 2000     // Reduced world data
#define VIEW_DISTANCE 2            // Minimum recommended
#define VISITED_HISTORY 2          // Reduce memory
#define TIME_BETWEEN_TICKS 2000000 // 0.5 TPS (slower)

// Disable heavy features
// #define DO_FLUID_FLOW           // Comment out water physics
// #define ALLOW_CHESTS            // Comment out if having issues
// #define BROADCAST_ALL_MOVEMENT  // Reduce network load
```

**Note:** Pico W has only 264 KB of RAM, so memory optimization is critical!

### 5. Build the Project

```bash
# Create build directory
mkdir build-pico
cd build-pico

# Configure CMake for Pico W
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPICO_BOARD=pico_w \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DCMAKE_TOOLCHAIN_FILE=$PICO_SDK_PATH/cmake/preload/toolchains/pico_arm_gcc.cmake \
      ..

# Copy the Pico CMakeLists
cp ../CMakeLists_pico.txt ../CMakeLists.txt

# Build
make -j4
```

This will generate:
- `bareiron.uf2` - Firmware file for flashing
- `bareiron.elf` - Debug symbols
- `bareiron.hex` - Alternative firmware format

### 6. Flash to Pico W

**Method 1: USB Bootloader (Recommended)**

1. Hold down the BOOTSEL button on the Pico W
2. Connect the Pico W to your computer via USB
3. Release the BOOTSEL button
4. The Pico W should appear as a USB drive (RPI-RP2)
5. Copy `bareiron.uf2` to the drive
6. The Pico W will automatically reboot and start running

**Method 2: Using OpenOCD (for development)**

```bash
# Install OpenOCD with Pico support
sudo apt install openocd

# Flash using OpenOCD
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg \
        -c "program bareiron.elf verify reset exit"
```

## Using the Server

### 1. Monitor Serial Output

Connect to the Pico W's USB serial port to see logs:

```bash
# Linux/Mac
screen /dev/ttyACM0 115200

# Or use minicom
minicom -D /dev/ttyACM0 -b 115200

# Windows: Use PuTTY or similar
```

You should see output like:
```
Bareiron starting on Raspberry Pi Pico W...
Connecting to WiFi SSID: your-wifi
WiFi connected successfully!
IP Address: 192.168.1.100
World seed (hashed): A103DE6C
RNG seed (hashed): 0E2B9419
Server listening on port 25565...
```

### 2. Connect from Minecraft

Use the IP address shown in the serial output:

1. Open Minecraft Java Edition
2. Multiplayer → Direct Connection
3. Enter the IP address (e.g., `192.168.1.100`)
4. Connect!

## Troubleshooting

### Build Issues

**"arm-none-eabi-gcc: command not found"**
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi
```

**"PICO_SDK_PATH not set"**
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

**"Could not find a package configuration file for PICO"**
- Make sure you've initialized Pico SDK submodules
- Run `git submodule update --init` in the pico-sdk directory

### Runtime Issues

**"Failed to connect to WiFi"**
- Verify SSID and password in `globals.h`
- Ensure you're using a 2.4 GHz network (not 5 GHz)
- Check if network uses WPA2 (other auth types may not work)
- Try moving closer to your router

**"No serial output"**
- Make sure you're using a data USB cable (not charge-only)
- Try a different USB port
- On Linux, you may need to add yourself to dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```

**"Server crashes/resets frequently"**
- Reduce MAX_PLAYERS and MAX_BLOCK_CHANGES
- Disable DO_FLUID_FLOW and ALLOW_CHESTS
- Increase TIME_BETWEEN_TICKS
- Check power supply (use a good quality USB cable and power source)

**"Can't connect from Minecraft"**
- Verify the IP address from serial output
- Make sure your computer and Pico are on the same network
- Check if port 25565 is blocked by firewall
- Try connecting from another device on the same network

### Memory Issues

If you get memory allocation errors:

1. Reduce `MAX_BLOCK_CHANGES` to 1000 or less
2. Set `MAX_PLAYERS` to 1 or 2
3. Disable `ALLOW_CHESTS`
4. Disable `DO_FLUID_FLOW`
5. Consider using overclocking (see below)

## Performance Optimization

### Overclocking (Advanced)

You can overclock the Pico W for better performance. Add to `CMakeLists_pico.txt`:

```cmake
# Overclock to 250 MHz (default is 125 MHz)
target_compile_definitions(bareiron PRIVATE
    PICO_BUILD=1
    PICO_DEFAULT_SYS_CLOCK_KHZ=250000
)
```

**Warning:** Overclocking may reduce stability and lifespan. Use at your own risk!

### Compiler Optimizations

The CMakeLists already uses `-O2` optimization. For more aggressive optimization:

```cmake
target_compile_options(bareiron PRIVATE
    -O3
    -flto
    -ffunction-sections
    -fdata-sections
)
```

## Advanced Topics

### World Persistence

By default, world data is lost when the Pico W is powered off. To enable persistence:

1. Set up LittleFS on Pico W flash
2. Uncomment `SYNC_WORLD_TO_DISK` in `globals.h`
3. Implement flash I/O using Pico SDK APIs

This requires additional development work.

### Using External Flash

For larger worlds, you can add external SPI flash:

1. Connect SPI flash chip to Pico W
2. Modify `serialize.c` to use external storage
3. Increase `MAX_BLOCK_CHANGES` accordingly

### Debugging

Enable debug output:

```c
#define DEV_LOG_UNKNOWN_PACKETS
#define DEV_LOG_LENGTH_DISCREPANCY
#define DEV_LOG_CHUNK_GENERATION
```

Use GDB for step-through debugging:

```bash
# In one terminal
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg

# In another terminal
arm-none-eabi-gdb bareiron.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset init
(gdb) continue
```

## Specifications

### Memory Usage
- Flash: ~200-300 KB (depending on configuration)
- RAM: ~200 KB (out of 264 KB available)

### Performance
- World generation: ~5-10 seconds per chunk
- Tick rate: 0.5-1 TPS (configurable)
- Max concurrent players: 1-2 recommended

### Network
- WiFi: 2.4 GHz 802.11n
- TCP connections: Limited by RAM
- Bandwidth: Sufficient for 1-2 players

## Resources

- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/)
- [Pico SDK Documentation](https://raspberrypi.github.io/pico-sdk-doxygen/)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [Pico W Datasheet](https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf)

## Contributing

Improvements to the Pico W port are welcome! Areas for improvement:

- [ ] Flash storage integration
- [ ] Better memory management
- [ ] Watchdog timer implementation
- [ ] Multi-core support
- [ ] External flash support
- [ ] Power management
- [ ] OTA (Over-The-Air) updates

See the main [README](README.md) for contribution guidelines.
