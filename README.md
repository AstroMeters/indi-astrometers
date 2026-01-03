# INDI Drivers for AstroMeters.eu

<p align="center">
  <img width="200" height="200" alt="AstroMeters Logo" src="https://github.com/user-attachments/assets/2135c8e6-5789-47cd-ac9b-e2287eecc98f" />
</p>

Official INDI drivers for **[AstroMeters.eu](https://astrometers.eu/)** devices – premium hardware for astronomy enthusiasts and professionals.
Our drivers enable seamless integration and full control of your telescope accessories and environmental monitoring devices.

---

## 🔭 Supported Devices

### AMFOC01 – Focuser

* **Description:** Precision focuser for telescopes, designed for demanding astronomical observations.
* **Key features:**

  * Absolute positioning with 1,000,000 step range
  * Hardware & software temperature compensation
  * Real-time monitoring (position, temperature)
  * Easy position synchronization

### AMSKY01 – Weather Station

* **Description:** Multi-sensor weather station for monitoring observing conditions.
* **Key features:**

  * Measures temperature, humidity, dew point, sky brightness (lux), and cloud coverage
  * Automatic weather data injection into FITS headers
  * Continuous data streaming via serial interface

**Variants:**

* **indi_amsky01** – Direct serial connection to AMSKY01 hardware
* **indi_amsky01_api** – HTTP API client that connects to `amsky01_viewer.py` service
  * No physical connection required
  * Fetches data from `http://localhost:8080/data.json` (configurable)
  * Ideal for remote monitoring or when the device is managed by another process

### AMTEST01 – Test Driver

* **Description:** Generic test driver for debugging and development.
* **Key features:**

  * Connects to any serial device
  * Prints all incoming data with timestamps
  * Simulation mode for offline development

---

## 📦 Installation

### Requirements

* Linux-based system (Stellarmate, Astroberry, Ubuntu, RPi OS, etc.)
* [INDI Library](https://indilib.org/) (`libindi-dev`)
* CMake ≥ 3.13

```bash
sudo apt install cmake libindi-dev libxisf-dev
```

### Build & Install

```bash
git clone https://github.com/AstroMeters/indi-astrometers.git
cd indi-astrometers
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

**Installed files:**

* Binaries: `/usr/bin/`
* XML driver definitions: `/usr/share/indi/`


## 🚀 Getting Started

1. Start your **INDI server** (e.g., with KStars/Ekos, CCDCiel, N.I.N.A.).
2. Connect your AstroMeters device(s) to your computer.
3. In your INDI client, locate AstroMeters drivers under the appropriate category (Focuser, Weather).
4. Connect and enjoy precise, reliable control over your observatory!


## 🙌 Contributing

We welcome community involvement! If you want to report a bug, request a feature, or submit code:

1. [Open an Issue](https://github.com/your-repo/indi-astrometers/issues) for bugs or feature suggestions.
2. Fork this repository.
3. Create a new branch (`feature/my-feature`).
4. Submit a Pull Request with a clear description.

Please follow the [INDI coding style](https://github.com/indilib/indi/blob/master/CONTRIBUTING.md) and include test cases where possible.


## 📧 Support

For questions, troubleshooting, or commercial support, visit [astrometers.eu](https://astrometers.eu) or email us at [info@astrometers.eu](mailto:info@astrometers.eu).

---

## 📄 License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
