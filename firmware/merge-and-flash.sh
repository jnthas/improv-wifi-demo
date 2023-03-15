PORT=${1-"/dev/ttyUSB0"}

esptool.py --chip ESP32 merge_bin -o improv-wifi-demo.bin --flash_mode dio --flash_size 4MB \
    0xe000 "/home/jonathas/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/partitions/boot_app0.bin" \
    0x1000 "/home/jonathas/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/bin/bootloader_qio_80m.bin" \
    0x10000 "/home/jonathas/.var/app/cc.arduino.IDE2/cache/arduino-sketch-5A27CA206F0A545A4BCAC48E8875278D/esp32-wifiimprov.ino.bin" \
    0x8000 "/home/jonathas/.var/app/cc.arduino.IDE2/cache/arduino-sketch-5A27CA206F0A545A4BCAC48E8875278D/esp32-wifiimprov.ino.partitions.bin"


esptool.py --chip esp32 --port $PORT --baud 921600 --before default_reset --after hard_reset \
    write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 improv-wifi-demo.bin 

