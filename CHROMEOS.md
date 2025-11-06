# Running Bareiron on ChromeOS

This guide explains how to run the Bareiron Minecraft server on ChromeOS without requiring Linux (Beta) or root privileges. We provide two main approaches:

## Option 1: WebAssembly (Browser-Based)

Run the Minecraft server directly in your web browser! This works on any ChromeOS device without special setup.

### Building the WebAssembly Version

**Prerequisites:**
- Emscripten SDK installed ([installation guide](https://emscripten.org/docs/getting_started/downloads.html))
- Node.js/Bun/Deno for building registries

**Steps:**

1. **Generate Registry Data** (if not already done):
   ```bash
   # On a system with Java installed
   ./extract_registries.sh
   # OR manually follow the README instructions
   ```

2. **Build the WASM Module**:
   ```bash
   # Make sure Emscripten is activated
   source /path/to/emsdk/emsdk_env.sh
   
   # Build the WebAssembly module
   ./build_wasm.sh
   ```

3. **Serve the Web Directory**:
   ```bash
   # Using Python
   cd web
   python3 -m http.server 8080
   
   # OR using Node.js
   npx http-server web -p 8080
   ```

4. **Open in Browser**:
   - Navigate to `http://localhost:8080` in your Chrome browser
   - Click "Start Server"
   - The Minecraft server will run in your browser!

### Connecting to the WebAssembly Server

Due to browser security restrictions, Minecraft clients cannot directly connect to a WebSocket server running in a browser tab. You have two options:

#### A. WebSocket-to-TCP Proxy (Recommended)

Use a proxy tool to bridge WebSocket and TCP:

```bash
# Install ws-tcp-relay
npm install -g ws-tcp-relay

# Run the proxy
ws-tcp-relay --listen 25565 --target ws://localhost:8080
```

Then connect your Minecraft client to `localhost:25565`.

#### B. Local Development Server

For testing, you can run a local proxy server that handles both HTTP serving and WebSocket-to-TCP bridging. Example proxy server code is available in `web/proxy-server.js` (to be created).

### Limitations of WebAssembly Version

- **World Persistence**: Currently limited. World data is stored in browser memory and will be lost when the page is closed or refreshed.
- **Performance**: May be slower than native execution, especially for world generation.
- **Memory**: Limited by browser memory restrictions (typically 2-4 GB).
- **Network**: Requires a WebSocket proxy for client connections.

### Advantages

- ✅ No installation required
- ✅ Works on any ChromeOS device
- ✅ No root privileges needed
- ✅ Can be deployed as a web app
- ✅ Cross-platform (works on any OS with a modern browser)

---

## Option 2: Raspberry Pi Pico W (External Hardware)

Use a Raspberry Pi Pico W microcontroller connected to your Chromebook to run the server.

### Why Pi Pico W?

- Very affordable (~$6 USD)
- Built-in WiFi
- Low power consumption
- No Linux required on ChromeOS
- Connects via USB to your Chromebook

### Building for Raspberry Pi Pico W

**Prerequisites:**
- Raspberry Pi Pico W board
- Pico SDK installed ([setup guide](https://github.com/raspberrypi/pico-sdk))
- ARM GCC toolchain
- CMake

**Steps:**

1. **Setup Pico SDK**:
   ```bash
   # Clone Pico SDK
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   export PICO_SDK_PATH=/path/to/pico-sdk
   ```

2. **Configure WiFi Credentials**:
   Edit `include/globals.h`:
   ```c
   #define WIFI_SSID "your-wifi-name"
   #define WIFI_PASS "your-wifi-password"
   ```

3. **Build for Pico W**:
   ```bash
   mkdir build-pico
   cd build-pico
   cmake -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=pico_w ..
   make -j4
   ```

4. **Flash to Pico W**:
   - Hold the BOOTSEL button on the Pico W
   - Connect it to your Chromebook via USB
   - Release BOOTSEL (should appear as USB drive)
   - Copy `bareiron.uf2` to the Pico W drive
   - The Pico will reboot and start the server

5. **Connect to Server**:
   - The Pico W will print its IP address via USB serial
   - Use a serial terminal to see the IP (Chrome Serial Terminal extension)
   - Connect your Minecraft client to that IP address on port 25565

### Configuring for Pico W

You may want to adjust these settings in `include/globals.h` for better Pico W performance:

```c
// Reduce memory usage
#define MAX_PLAYERS 4           // Fewer players
#define MAX_BLOCK_CHANGES 5000  // Less world data
#define VIEW_DISTANCE 2         // Keep this low
#define VISITED_HISTORY 2       // Reduce memory

// Optimize performance
#define TIME_BETWEEN_TICKS 2000000  // Slower tick rate (0.5 TPS)
// #define DO_FLUID_FLOW           // Comment out to disable water physics
// #define ALLOW_CHESTS            // Comment out if having issues
```

### Advantages

- ✅ True embedded hardware solution
- ✅ Runs independently of Chromebook
- ✅ Low cost (~$6)
- ✅ Can run 24/7 with minimal power
- ✅ Real TCP networking (no proxy needed)

### Limitations

- Requires purchasing hardware
- Limited RAM (264 KB)
- Requires USB connection for power
- Configuration requires rebuilding firmware

---

## Option 3: Android App (If Available)

Some ChromeOS devices support Android apps. If your device does, you could potentially:

1. Build an Android APK using Android NDK
2. Install it through the Play Store or side-load it
3. Run the server as an Android background service

This option is not yet implemented but is feasible.

---

## Comparison Table

| Feature | WebAssembly | Pico W | Android App |
|---------|-------------|--------|-------------|
| Cost | Free | ~$6 | Free |
| Setup Difficulty | Easy | Moderate | Easy |
| Performance | Good | Limited | Good |
| Memory | 2-4 GB | 264 KB | 1-4 GB |
| World Persistence | Browser only | No* | Yes |
| Network Type | WebSocket | TCP | TCP |
| Requires Proxy | Yes | No | No |
| 24/7 Operation | No | Yes | Yes |

*World persistence on Pico W requires additional flash storage setup

---

## Recommended Approach

**For quick testing and experimentation:**
→ Use the WebAssembly version

**For a permanent, low-cost server:**
→ Use the Raspberry Pi Pico W

**For best performance on ChromeOS:**
→ Wait for Android app support (coming soon)

---

## Troubleshooting

### WebAssembly Issues

**"bareiron.js not found"**
- Make sure you've run `./build_wasm.sh` successfully
- Check that `web/bareiron.js` and `web/bareiron.wasm` exist

**"Failed to load WASM module"**
- Ensure you're serving the files over HTTP (not file://)
- Check browser console for specific error messages

**Minecraft client can't connect**
- Make sure you're running the WebSocket proxy
- Verify the proxy is listening on the correct port
- Check that your firewall isn't blocking connections

### Pico W Issues

**Pico W won't connect to WiFi**
- Double-check SSID and password in `globals.h`
- Ensure you're using a 2.4 GHz network (Pico W doesn't support 5 GHz)
- Try moving closer to your router

**Can't see serial output**
- Install Chrome Serial Terminal extension
- Connect to the Pico W's serial port (usually 115200 baud)
- Try a different USB cable if no serial port appears

**Minecraft client can't connect**
- Verify the IP address printed by the Pico W
- Make sure your Chromebook and Pico W are on the same network
- Check that port 25565 isn't blocked by your router

---

## Additional Resources

- [Bareiron Main README](README.md)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [Pico W Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)
- [Minecraft Protocol Documentation](https://minecraft.wiki/w/Java_Edition_protocol)

---

## Contributing

If you improve the ChromeOS support, please contribute back! See the main README for contribution guidelines.
