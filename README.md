# Hourly Waveshare R2 frame

Replacement Arduino sketch for the **Waveshare ESP32-S3-ePaper-13.3E6** all-in-one board. This is reconstructed source, not the original recovered program.

## Setup

1. Keep this entire folder together. Open `eink.ino` in Arduino IDE.
2. Install **esp32 by Espressif Systems, version 3.2.0** in Boards Manager. No additional libraries are needed.
3. For a fresh checkout, copy `config.example.h` to `config.h`. Existing local settings are preserved and excluded from Git. Edit `config.h`: enter your 2.4 GHz Wi-Fi name and password. The supplied R2 URL is already configured.
4. Select **ESP32S3 Dev Module** and these Tools settings, following Waveshare's example configuration:

   | Setting | Value |
   | --- | --- |
   | USB CDC On Boot | Enabled |
   | Flash Size | 32MB (256Mb) |
   | Partition Scheme | 32M Flash (4.8MB APP/22MB FATFS) |
   | PSRAM | OPI PSRAM |
   | Flash Mode | QIO 80MHz |
   | Upload Mode | UART0 / Hardware CDC |

5. Select the connected board's port, then upload. Open Serial Monitor at **115200 baud** for status. Uploading replaces its existing firmware; back it up first if you want to preserve it.

If the module printed on your physical board is a different revision (for example N16R8 rather than N32R16V), match its flash/PSRAM settings instead. The current Waveshare documentation contains both module descriptions.

## Image and rotation

The supplied `current.sp6` endpoint was fetched and inspected. It contains **960,000 bytes**, with no header: **1200 columns × 1600 rows**, two pixels per byte, left pixel in the high nibble. The replacement image is the portrait LLM reading list. Every pixel uses a supported panel color:

| Nibble | Color |
| --- | --- |
| 0 | Black |
| 1 | White |
| 2 | Yellow |
| 3 | Red |
| 5 | Blue |
| 6 | Green |

The default sends those portrait pixels directly to the official panel driver, without an extra rotation. If the picture is upside down on the mounted frame, set `ROTATE_180 = true` in `config.h`. Physical mounting must be checked on your board; it cannot be determined from the URL alone. A landscape or PNG replacement must be converted to this exact native pixel layout before uploading to R2. This sketch does not decode PNG or discover a URL from a bucket listing.

## Hourly operation

Each boot or timer wake connects to Wi-Fi, synchronizes time for HTTPS certificate verification, downloads and validates the whole file into PSRAM, shuts down Wi-Fi, refreshes the display, and powers down the panel. ESP32 deep sleep keeps the timer running. The next wake is scheduled for approximately one hour after the start of the current wake, subtracting download/refresh time. This is an interval schedule, not alignment to the top of the clock hour.

Every successful hourly fetch triggers a display refresh, including unchanged images. Failed Wi-Fi, TLS, HTTP, size, pixel validation, or incomplete download leaves the existing picture and returns to sleep. Panel BUSY waits have a two-minute timeout per stage to prevent an indefinite battery drain; a power or display fault during an actual refresh can still interrupt the picture.

The client verifies HTTPS against the bundled ISRG Root X1/X2 certificates. It requires HTTP 200 and Content-Length 960000, and does not follow redirects. If R2 changes its certificate authority, update `RootCA.h` with the relevant trusted root.

Battery operation requires power to remain present while the ESP32 sleeps. If a USB power bank switches off at low current, the board cannot wake itself until power returns; use an always-on pack or a suitable battery on the board's battery connector. Actual sleep current and battery life require measurement on your hardware.

## Driver provenance

Bundled `DEV_Config.*` and `EPD_13in3e.*` come from [Waveshare's official examples](https://github.com/waveshareteam/ESP32-S3-ePaper-13.3E6), commit `f8e9456bd1ca520f638786142fc17a74154f4e05`, directory `example/Arduino-3.2.0/examples/03_E-Paper_Example`. Original notices and the upstream license are included. Local changes to the panel driver add bounded BUSY waits, refresh status, and a wait for power-off to finish. Pin assignments and register initialization remain from the official example.

[Waveshare Arduino setup](https://docs.waveshare.com/ESP32-S3-ePaper-13.3E6/Arduino) · [ISRG root certificates](https://letsencrypt.org/certificates/)

## Validation

See `VALIDATION.md` for compilation and data checks. The user confirmed the local color-band test and R2 fetch mode work. Hourly timer wake and battery current still require validation on the board.

## Production configuration

Development color-band and five-second fetch loops have been removed. REFRESH_SECONDS is 3600. The frame wakes, connects to Wi-Fi, fetches and validates the image, refreshes the panel, powers down Wi-Fi and the panel, and returns to deep sleep. Successful hourly fetches refresh even unchanged images. Failed downloads retain the previous picture and return to sleep.

Logs remain available at 115200 baud on UART0 through the USB bridge and native USB when connected. The startup marker is `Hourly EINK frame starting`. A serial monitor is not required for operation.

## Final battery check

Upload the updated eink.ino. Confirm the image appears and the monitor prints `Refresh complete.` then `Sleeping for ... seconds` (slightly less than 3600). Leave it powered and confirm another wake about one hour later. Repeat on battery with USB disconnected. If a USB power bank cuts power at low current, the board cannot wake until power returns.

## Committing

Commit the sketch, driver files, FrameConsole files, RootCA.h, config.example.h, .gitignore, and documentation. Keep config.h and compiled binaries local because they contain Wi-Fi settings. No Git repository or commit was created by this task.
