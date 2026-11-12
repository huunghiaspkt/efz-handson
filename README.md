# efz-handson

Hands-on Zephyr examples for the
[EFZ-ESP32S3 board](https://embeddedfun.org/docs/zephyr-training/basic/efz-esp32s3-board/).

This repository is a west manifest, laid out like Zephyr's
[example-application](https://github.com/zephyrproject-rtos/example-application) (west T2):
`west.yml` brings in upstream Zephyr and only the modules this repository needs, and the repository is a
Zephyr module that adds the EFZ-ESP32S3 board (`boards/`) and its devicetree vendor prefix (`dts/`).

## Getting Started

Before getting started, set up Zephyr's tools (Python, west, the Zephyr SDK) as in
[How to Start](https://embeddedfun.org/docs/zephyr-training/how-to-start).

### Initialization

Initialize the workspace folder (`efz-handson-ws`), where `efz-handson`, Zephyr and the modules
will be cloned:

```shell
# Python environment with west, inside the workspace folder
python3 -m venv efz-handson-ws/.venv
source efz-handson-ws/.venv/bin/activate
pip install west
# initialize efz-handson-ws for efz-handson (main branch)
west init -m https://github.com/huunghiaspkt/efz-handson --mr main efz-handson-ws
# update Zephyr and the modules (latest commit only, no git history)
cd efz-handson-ws
west update --narrow -o=--depth=1
west packages pip --install
west blobs fetch hal_espressif
```

### Building and running

```shell
cd efz-handson
west build -b efz_esp32s3/esp32s3/procpu app
west flash
```

A debug configuration is also provided:

```shell
west build -b efz_esp32s3/esp32s3/procpu app -- -DEXTRA_CONF_FILE=debug.conf
```

### Testing

```shell
west twister -T app --integration     # build the app
west twister -T tests --integration   # run the tests
```

## Layout

Same as [example-application](https://github.com/zephyrproject-rtos/example-application),
with its demo code removed and the EFZ-ESP32S3 board in `boards/`:

```
app/                                    the application
app/src/lcd/                            the LCD: lcd_show_*() and its pages
app/src/modules/                        one folder per part of the board (env: SHT30, io: buttons and LEDs)
app/boards/                             the 1.8" ST7735 LCD overlay and its wiring
boards/embeddedfun/efz_esp32s3/         the EFZ-ESP32S3 board
drivers/                                out-of-tree drivers
dts/bindings/                           devicetree bindings, "embeddedfun" vendor prefix
include/app/                            public headers for drivers/ and lib/
lib/                                    out-of-tree libraries
scripts/                                west commands and runners
tests/                                  Twister tests
zephyr/module.yml                       makes this repository a Zephyr module
```
